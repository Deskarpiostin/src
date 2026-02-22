# hl2sbpp-wrapper.nix
{ writeShellScriptBin
, getopt
, xorg
, hl2-unwrapped
}:

writeShellScriptBin "hl2sbpp-wrapper" ''
  config_path=$HOME/.config/hl2sbpp/config
  default_resource_path=$HOME/hl2sbpp

  longoptions="resource-path:,impermanent,help"
  shortoptions="r:h"

  impermanent=false

  usage() {
    echo "hl2sbpp(-wrapper) usage:"
    echo "Define resource_path variable in $config_path, with the argument shown below or put HL2 resource folders in $default_resource_path."
    grep ' \+-.*) ' $0 | sed 's/#//' | sed -r 's/([a-z])\)/\1/'
    exit 0
  }

  save_newly_created_files() {
    newly_created_files=$(cd $tmp_dir && find . -type f | cut -c 2-)
    echo "$newly_created_files" | while read file; do
      cp $tmp_dir$file $resource_path$file
    done
  }

  cleanup() {
    if ! $impermanent; then
      save_newly_created_files
    fi
    rm -rf $tmp_dir
  }

  if [[ -e $config_path ]]; then
    source $config_path
  fi

  parsed=$(${getopt}/bin/getopt -l $longoptions -o $shortoptions -a -- "$@")
  eval set -- "$parsed"

  while true; do
    case "$1" in
      -r | --resource-path)
        resource_path="$2"
        shift 2
        ;;
      --impermanent)
        impermanent=true
        shift 1
        ;;
      -h | --help)
        usage
        ;;
      --)
        shift
        break
        ;;
      *)
        echo "Wrong argument: $1"
        usage
        ;;
    esac
  done

  if [[ ! $resource_path ]]; then
    echo "Resource path not set, defaulting to $default_resource_path"
    resource_path=$default_resource_path
  fi

  if [[ ! -d $resource_path/hl2 || ! -d $resource_path/platform ]]; then
    echo "You must have 'hl2' and 'platform' folders in $resource_path"
    exit 1
  fi

  tmp_dir=$(mktemp -d)
  echo "Using temp directory: $tmp_dir"

  mkdir $tmp_dir/{hl2,platform,hl2sbpp}

  ln -s $resource_path/hl2/* $tmp_dir/hl2/
  ln -s $resource_path/platform/* $tmp_dir/platform/
  rm -rf $tmp_dir/hl2/bin

  # clone our repo if it doesn't exist in temp
  HL2SBPP_REPO="https://github.com/hl2sbpp/hl2sbpp.git"
  if [[ ! -d $tmp_dir/hl2sbpp ]]; then
    git clone $HL2SBPP_REPO $tmp_dir/hl2sbpp
  fi

  ${xorg.lndir}/bin/lndir "${hl2sbpp-unwrapped}" $tmp_dir

  trap cleanup EXIT

  (cd $tmp_dir ; ./hl2sbpp_launcher $launcher_parameters)
''
