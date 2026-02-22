//========== Copyright (C) 2026, Team HL2SB++, All rights reserved. ===========//
//
// Purpose:
//
//===========================================================================//

#include "imageextbutton.h"
#include "vgui/IInput.h"
#include "vgui/ISurface.h"
#include "tier1/KeyValues.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

std::unordered_map< std::string, TexInfo > ImageExtButton::s_textureCache;

ImageExtButton::ImageExtButton( vgui::Panel *parent, const char *panelName, const char *normalImage, const char *mouseOverImage, const char *mouseClickImage, const char *pCmd ) :
	vgui::Panel( parent, panelName ),
	m_hasMouseOverImage( false ),
	m_hasMouseClickImage( false ),
	m_hasCommand( false ),
	m_bScaleImage( true ),
	m_currentImage( nullptr ),
	m_pParent( parent )
{
	Q_strncpy( m_normalImagePath, normalImage ? normalImage : "", sizeof( m_normalImagePath ) );
	Q_strncpy( m_mouseOverImagePath, mouseOverImage ? mouseOverImage : "", sizeof( m_mouseOverImagePath ) );
	Q_strncpy( m_mouseClickImagePath, mouseClickImage ? mouseClickImage : "", sizeof( m_mouseClickImagePath ) );
	Q_strncpy( m_command, pCmd ? pCmd : "", sizeof( m_command ) );

	m_hasMouseOverImage = ( mouseOverImage && mouseOverImage[0] != '\0' );
	m_hasMouseClickImage = ( mouseClickImage && mouseClickImage[0] != '\0' );
	m_hasCommand = ( pCmd && pCmd[0] != '\0' );

	if ( m_normalImagePath[0] != '\0' )
		LoadImage( m_normalImagePath, m_normalImage );

	if ( m_hasMouseOverImage && m_mouseOverImagePath[0] != '\0' )
		LoadImage( m_mouseOverImagePath, m_mouseOverImage );

	if ( m_hasMouseClickImage && m_mouseClickImagePath[0] != '\0' )
		LoadImage( m_mouseClickImagePath, m_mouseClickImage );

	SetNormalImage();
}

ImageExtButton::~ImageExtButton()
{
	if ( m_normalImage.textureId != -1 )
		ReleaseTextureByKey( m_normalImagePath );
	if ( m_mouseOverImage.textureId != -1 )
		ReleaseTextureByKey( m_mouseOverImagePath );
	if ( m_mouseClickImage.textureId != -1 )
		ReleaseTextureByKey( m_mouseClickImagePath );
}

int ImageExtButton::NextPowerOfTwo( int value )
{
	if ( value <= 0 )
		return 1;
	if ( ( value & ( value - 1 ) ) == 0 )
		return value; // already power of 2

	int power = 1;
	while ( power < value )
	{
		power <<= 1;
	}
	return power;
}

unsigned char *ImageExtButton::ResizeImageToPowerOfTwo( unsigned char *originalData, int originalWidth, int originalHeight, int &newWidth, int &newHeight )
{
	newWidth = NextPowerOfTwo( originalWidth );
	newHeight = NextPowerOfTwo( originalHeight );

	if ( newWidth == originalWidth && newHeight == originalHeight )
	{
		return originalData;
	}

	unsigned char *newData = (unsigned char *)malloc( newWidth * newHeight * 4 );
	if ( !newData )
		return originalData;

	const float xScale = (float)originalWidth / (float)newWidth;
	const float yScale = (float)originalHeight / (float)newHeight;

	for ( int y = 0; y < newHeight; ++y )
	{
		float srcY = ( y + 0.5f ) * yScale - 0.5f;
		int y0 = floorf( srcY );
		int y1 = y0 + 1;
		float fy = srcY - (float)y0;

		if ( y0 < 0 ) { y0 = 0; fy = srcY; }
		if ( y1 < 0 ) y1 = 0;
		if ( y0 >= originalHeight ) { y0 = originalHeight - 1; fy = 0.0f; }
		if ( y1 >= originalHeight ) y1 = originalHeight - 1;

		for ( int x = 0; x < newWidth; ++x )
		{
			float srcX = ( x + 0.5f ) * xScale - 0.5f;
			int x0 = floorf( srcX );
			int x1 = x0 + 1;
			float fx = srcX - (float)x0;

			if ( x0 < 0 ) { x0 = 0; fx = srcX; }
			if ( x1 < 0 ) x1 = 0;
			if ( x0 >= originalWidth ) { x0 = originalWidth - 1; fx = 0.0f; }
			if ( x1 >= originalWidth ) x1 = originalWidth - 1;

			unsigned char *p00 = &originalData[( y0 * originalWidth + x0 ) * 4];
			unsigned char *p10 = &originalData[( y0 * originalWidth + x1 ) * 4];
			unsigned char *p01 = &originalData[( y1 * originalWidth + x0 ) * 4];
			unsigned char *p11 = &originalData[( y1 * originalWidth + x1 ) * 4];

			for ( int c = 0; c < 4; ++c )
			{
				float i0 = p00[c] * (1.0f - fx) + p10[c] * fx;
				float i1 = p01[c] * (1.0f - fx) + p11[c] * fx;

				float val = i0 * (1.0f - fy) + i1 * fy;

				int dstIdx = ( y * newWidth + x ) * 4 + c;
				int iv = (int)( val + 0.5f );
				if ( iv < 0 ) iv = 0;
				if ( iv > 255 ) iv = 255;
				newData[dstIdx] = (unsigned char)iv;
			}
		}
	}

	return newData;
}

bool ImageExtButton::LoadImage( const char *filename, ImageData &imageData )
{
	if ( !filename || filename[0] == '\0' )
		return false;

	CUtlBuffer buf;
	if ( !g_pFullFileSystem->ReadFile( filename, "MOD", buf ) )
	{
		Warning( "ImageExtButton: Could not open image file: %s\n", filename );
		return false;
	}

	int originalWidth = 0, originalHeight = 0, channels = 0;

	unsigned char *originalData = stbi_load_from_memory( reinterpret_cast< unsigned char * >( buf.Base() ), buf.TellPut(), &originalWidth, &originalHeight, &channels,
		4 // force RGBA
	);

	if ( !originalData )
	{
		Warning( "ImageExtButton: Failed to decode '%s' (%s)\n", filename, stbi_failure_reason() );
		return false;
	}

	if ( originalWidth <= 0 || originalHeight <= 0 || originalWidth > 4096 || originalHeight > 4096 )
	{
		Warning( "ImageExtButton: Invalid image dimensions %dx%d for '%s'\n", originalWidth, originalHeight, filename );
		stbi_image_free( originalData );
		return false;
	}

	int			   textureWidth, textureHeight;
	unsigned char *textureData = ResizeImageToPowerOfTwo( originalData, originalWidth, originalHeight, textureWidth, textureHeight );

	bool needsFreeing = ( textureData != originalData );

	Msg( "ImageExtButton: Loaded '%s' - Original: %dx%d, Texture: %dx%d\n", filename, originalWidth, originalHeight, textureWidth, textureHeight );

	imageData.textureId = CreateOrGetTextureFromImageData( filename, textureData, textureWidth, textureHeight );
	imageData.width = originalWidth;
	imageData.height = originalHeight;
	imageData.isValid = ( imageData.textureId != -1 );

	// Clean up
	if ( needsFreeing )
	{
		free( textureData );
	}
	stbi_image_free( originalData );

	return imageData.isValid;
}

int ImageExtButton::CreateOrGetTextureFromImageData( const char *key, unsigned char *data, int width, int height )
{
	std::string keyStr( key ? key : "" );
	auto		it = s_textureCache.find( keyStr );
	if ( it != s_textureCache.end() )
	{
		it->second.refCount++;
		return it->second.texId;
	}

	int texId = vgui::surface()->CreateNewTextureID( true );
	vgui::surface()->DrawSetTextureRGBA( texId, data, width, height, 1, false );
	TexInfo texInfo( texId, width, height, 1 );
	s_textureCache[keyStr] = texInfo;
	return texId;
}

void ImageExtButton::ReleaseTextureByKey( const char *key )
{
	if ( !key || key[0] == '\0' )
		return;

	std::string keyStr( key );
	auto		it = s_textureCache.find( keyStr );
	if ( it != s_textureCache.end() )
	{
		it->second.refCount--;
		if ( it->second.refCount <= 0 )
		{
			vgui::surface()->DestroyTextureID( it->second.texId );
			s_textureCache.erase( it );
		}
	}
}

bool ImageExtButton::LoadImageIfNeeded( ImageData &imageData, const char *path, bool &flag )
{
	if ( !imageData.isValid && path && path[0] != '\0' )
	{
		flag = LoadImage( path, imageData );
		return flag;
	}
	return false;
}

void ImageExtButton::SetCurrentImage( ImageData *image )
{
	m_currentImage = image;

	if ( m_currentImage && m_currentImage->isValid && !m_bScaleImage )
	{
		if ( m_currentImage->width > 0 && m_currentImage->height > 0 )
		{
			SetSize( m_currentImage->width, m_currentImage->height );
		}
	}

	InvalidateLayout();
	Repaint();
}

void ImageExtButton::SetNormalImage()
{
	if ( !m_normalImage.isValid && m_normalImagePath[0] != '\0' )
	{
		LoadImage( m_normalImagePath, m_normalImage );
	}
	SetCurrentImage( &m_normalImage );
}

void ImageExtButton::SetMouseOverImage()
{
	if ( m_hasMouseOverImage && !m_mouseOverImage.isValid && m_mouseOverImagePath[0] != '\0' )
	{
		LoadImage( m_mouseOverImagePath, m_mouseOverImage );
	}
	SetCurrentImage( &m_mouseOverImage );
}

void ImageExtButton::SetMouseClickImage()
{
	if ( m_hasMouseClickImage && !m_mouseClickImage.isValid && m_mouseClickImagePath[0] != '\0' )
	{
		LoadImage( m_mouseClickImagePath, m_mouseClickImage );
	}
	SetCurrentImage( &m_mouseClickImage );
}

void ImageExtButton::Paint()
{
	if ( !m_currentImage )
		return;

	if ( m_currentImage->isValid )
	{
		vgui::surface()->DrawSetTexture( m_currentImage->textureId );
		vgui::surface()->DrawSetColor( Color( 255, 255, 255, 255 ) );

		if ( m_bScaleImage )
		{
			vgui::surface()->DrawTexturedRect( 0, 0, GetWide(), GetTall() );
		}
		else
		{
			int w = m_currentImage->width;
			int h = m_currentImage->height;
			vgui::surface()->DrawTexturedRect( 0, 0, w, h );
		}
	}
}

void ImageExtButton::OnCursorEntered()
{
	if ( m_hasMouseOverImage )
		SetMouseOverImage();
}

void ImageExtButton::OnCursorExited()
{
	SetNormalImage();
}

void ImageExtButton::OnMousePressed( vgui::MouseCode code )
{
	if ( m_hasMouseClickImage )
		SetMouseClickImage();
}

void ImageExtButton::OnMouseReleased( vgui::MouseCode code )
{
	if ( m_hasMouseOverImage )
	{
		SetMouseOverImage();
	}
	else
	{
		SetNormalImage();
	}

	if ( m_hasCommand )
	{
		PostActionSignal( new KeyValues( "Command", "command", m_command ) );
	}
}

void ImageExtButton::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

void ImageExtButton::OnCommand( KeyValues *data )
{
	const char *command = data->GetString( "command", "" );
	if ( m_pParent )
	{
		m_pParent->OnCommand( command );
	}
}