#!/bin/sh

# ================= NO CHANGE NEEDED BELOW! =====================

cache_dir=".tuxera_update_cache"
script_version="12.8.16"
server="autobuild.tuxera.com"

usage() {
    usagestr=$(cat <<EOF
Usage: tuxera_update.sh [OPTION...]

  This script only assembles the kernel headers package
  (kheaders.tar.bz2) by default. Use -a to invoke Autobuild.

  --source-dir SRCDIR [--output-dir OUTDIR]

      Specify kernel source and optionally, kernel build output
      directories. This is necessary if kernel headers package assembly
      is performed.

  --no-excludes

      Do not exclude *.c, *.o, *.S, arch/*/boot. Only use if the build
      fails (some of the excluded files are needed). This significantly
      grows the headers package size.

  -a [--target TARGET] [--user USER] [--pass PASSWD] [MODE] [OPTIONS]

      Start Autobuild against TARGET. If target was not specified, uses
      the default target. Use '--target list' to show available targets.

      If --user or --pass is not specified, they will be read from stdin.
      NOTE: Using --pass can be dangerous as local users see it in 'ps'.

      Autobuild starts with .tar.bz2 assembly by default, but in MODE you
      can optionally specify another operating mode:

        --use-package PACKAGE
          Start Autobuild by uploading a pre-built .tar.bz2 file PACKAGE.

        --use-remote-package PACKAGE
          Start Autobuild with a previously uploaded .tar.bz2 file PACKAGE.

      The following extra options are supported:

        --use-cache
          Do not invoke a remote build if kernel dependencies are not
          modified. Creates or updates the cache after a remote build.
          You must provide --source-dir (optionally --output-dir) to
          use the cache. Ensure the toolchain is in path.

        --cache-dir CACHEDIR
          Specify local cache directory. The default directory is
          \$PWD/.tuxera_update_cache

        --ignore-cert
          If up/download fails due to certificate issues, this option can
          be used to disable verification.

        --use-curl
        --use-wget
          Force the use of 'curl' or 'wget' for remote communication. Due
          to 'wget' limitations, 'sftp' is required with 'wget'.

        --version SOFTWARE=VERSION[,...]
          Example: --version NTFS=3012.4.2,EXFAT=3012.4.9
          Specify the version of software component(s) to build.

  -h, --help

      Prints this help.

  Tuxera Autobuild Team
  autobuild-support@tuxera.com
EOF
)

  if [ -n "$long_help" ] ; then
      echo "$usagestr"
      echo
  else
      echo "For long help, use -h. Options:"
      echo "$usagestr" | grep "^ *-"
  fi
  exit 1
}

build_package() {
    if [ -z "$1" ] ; then
        echo "You must specify --source-dir for headers package assembly."
        usage
    fi

    # First make sure the Linux kernel source directory is set correctly. 
    # To do so, let's check the COPYRIGHT file because that is always included.

    # Let's guess if the paths are relative or absolute.
    CUR_DIR="$(pwd)"
    KERNEL_ABS=$(expr index $1 /)
    # If the first character is '/', then the path is absolute.
    if [ "${KERNEL_ABS}" = "1" ] ; then KERNEL_DIR="$1"; else KERNEL_DIR="${CUR_DIR}/$1"; fi
    if [ ! -z "$2" ] ; then
        OUTPUT_ABS=$(expr index $2 /)
        if [ "${OUTPUT_ABS}" = "1" ] ; then OUTPUT_DIR="$2"; else OUTPUT_DIR="${CUR_DIR}/$2"; fi
    fi

    KERNEL_LINK=$(mktemp)

    if [ $? -ne 0 ] ; then
        echo "mktemp failed. Unable to continue."
        exit 1
    fi

    ln -sf "${KERNEL_DIR}" "${KERNEL_LINK}"

    if [ ! -z "${OUTPUT_DIR}" ] ; then
        OUTPUT_LINK=$(mktemp)

        if [ $? -ne 0 ] ; then
            echo "mktemp failed. Unable to continue."
            rm -f "${KERNEL_LINK}"
            exit 1
        fi

        ln -sf "${OUTPUT_DIR}" "${OUTPUT_LINK}"
    fi

    if [ $? -ne 0 ] ; then
        echo "Symlinking (ln -s) failed. Unable to continue."
        rm -f "${KERNEL_LINK}"; if [ ! -z "${OUTPUT_DIR}" ] ; then rm -f "${OUTPUT_LINK}"; fi
        exit 1
    fi

    if test ! -e "${KERNEL_LINK}/COPYING"; then
        echo "  ERROR: Kernel source code directory is invalid (no COPYING found).";
        echo "         To fix it, set the --source-dir parameter correctly.";

        rm "${KERNEL_LINK}"; if [ ! -z "${OUTPUT_DIR}" ] ; then rm "${OUTPUT_LINK}"; fi
        exit 1
    fi

    if test ! -e "${KERNEL_LINK}/include/config/auto.conf" -a ! -e "${OUTPUT_LINK}/include/config/auto.conf"; then
            echo "  ERROR: Invalid kernel configuration:";
            echo "         include/config/auto.conf is missing.";
            echo "         To fix it run 'make oldconfig && make modules_prepare' and ";
            echo "         'make sure ARCH= and CROSS_COMPILE= are correctly set and exported.";

        rm "${KERNEL_LINK}"; if [ ! -z "${OUTPUT_DIR}" ] ; then rm "${OUTPUT_LINK}"; fi
        exit 1
    fi

    if test -e "${KERNEL_LINK}/Module.symvers"; then
        KSYMVERS=${KERNEL_LINK}/Module.symvers
    fi
    if test ! -z "${OUTPUT_LINK}" -a -e "${OUTPUT_LINK}/Module.symvers"; then
        OSYMVERS=${OUTPUT_LINK}/Module.symvers
    fi
    if test ! -e "${KSYMVERS}" -a ! -e "${OSYMVERS}"; then
            echo "  ERROR: Invalid kernel configuration:";
            echo "         Module.symvers is missing.";
            echo "         To fix it run 'make oldconfig && make modules_prepare && make' and ";
            echo "         'make sure ARCH= and CROSS_COMPILE= are correctly set and exported.";

        rm "${KERNEL_LINK}"; if [ ! -z "${OUTPUT_DIR}" ] ; then rm "${OUTPUT_LINK}"; fi
        exit 1
    fi

    if test -e "${KERNEL_LINK}/.config"; then
        KCONFIG=${KERNEL_LINK}/.config
    fi

    if test ! -z "${OUTPUT_LINK}"; then
        if test -e "${OUTPUT_LINK}/.config"; then
            OCONFIG=${OUTPUT_LINK}/.config
        fi
        if test -e "${OUTPUT_LINK}/Makefile"; then
            OMAKEFILE=${OUTPUT_LINK}/Makefile
        fi
        if test -e "${OUTPUT_LINK}/scripts"; then
            OSCRIPTS=${OUTPUT_LINK}/scripts
        fi
        if test -e "${OUTPUT_LINK}/arch"; then
            OARCH=${OUTPUT_LINK}/arch
        fi
        if test -e "${OUTPUT_LINK}/include"; then
            OINCLUDE=${OUTPUT_LINK}/include
        fi
    fi

    echo "Generating list of files to include..."
    INCLUDEFILE=$(mktemp)

    if [ $? -ne 0 ] ; then
        echo "mktemp failed. Unable to continue."
        rm "${KERNEL_LINK}"; if [ ! -z "${OUTPUT_DIR}" ] ; then rm "${OUTPUT_LINK}"; fi
        exit 1
    fi

    SEARCHPATHS="${KERNEL_LINK}/include ${KERNEL_LINK}/arch ${KERNEL_LINK}/scripts"
    if [ ! -z "${OUTPUT_DIR}" ] ; then SEARCHPATHS="${SEARCHPATHS} ${OSCRIPTS} ${OARCH} ${OINCLUDE}"; fi

    if [ -n "$no_excludes" ] ; then
        find -L ${SEARCHPATHS} ! -type l >> ${INCLUDEFILE}
    else
        find -L ${SEARCHPATHS} \
            \( ! -type l -a ! -name \*.c -a ! -name \*.o -a ! -name \*.S -a ! -path \*/arch/\*/boot/\* -a ! -path \*/.svn/\* -a ! -path \*/.git/\* \) \
            >> ${INCLUDEFILE}
    fi

    echo ${KERNEL_LINK}/Makefile >> ${INCLUDEFILE}
    if [ -n "${KCONFIG}" ]; then echo ${KCONFIG} >> ${INCLUDEFILE}; fi
    if [ -n "${KSYMVERS}" ]; then echo ${KSYMVERS} >> ${INCLUDEFILE}; fi
    if [ -n "${OMAKEFILE}" ]; then echo ${OMAKEFILE} >> ${INCLUDEFILE}; fi
    if [ -n "${OCONFIG}" ]; then echo ${OCONFIG} >> ${INCLUDEFILE}; fi
    if [ -n "${OSYMVERS}" ]; then echo ${OSYMVERS} >> ${INCLUDEFILE}; fi

    echo "Packing kernel headers ..."
    tar cjf "${3}" --dereference --no-recursion --files-from "${INCLUDEFILE}"

    if [ $? -ne 0 ] ; then
        echo "'tar cjf ${3} ...' failed. I will now exit."
        rm "${KERNEL_LINK}"; if [ ! -z "${OUTPUT_DIR}" ] ; then rm "${OUTPUT_LINK}"; fi
        exit 1
    fi

    rm ${INCLUDEFILE}
    rm "${KERNEL_LINK}"; if [ ! -z "${OUTPUT_DIR}" ] ; then rm "${OUTPUT_LINK}"; fi

    echo "Headers package assembly succeeded. You could now use --use-package ${3}."
}

upload_package_curl() {
    echo "Uploading the following package:"
    ls -lh "${1}"

    reply=$($curl -F "file=@${1}" https://${server}/upload.php)

    if [ $? -ne 0 ] ; then
        echo "curl failed. Unable to continue."
        exit 1
    fi

    status=$(echo "$reply" | head -n 1)

    if [ "$status" != "OK" ] ; then
        echo "Upload failed. Unable to continue."
        exit 1
    fi

    remote_package=$(echo "$reply" | head -n 2 | tail -n 1)

    echo "Upload succeeded. You could now use --use-remote-package ${remote_package}."
}

upload_package_sftp() {
    echo "Downloading SFTP key..."

    keyfile=$(mktemp)

    if [ $? -ne 0 ] ; then
        echo "mktemp failed. Unable to continue."
        exit 1
    fi

    $wget -O "$keyfile" https://${server}/id.php

    if [ $? -ne 0 ] ; then
        echo "Failed to download SFTP key. Unable to continue."
        exit 1
    fi

    chmod 400 "$keyfile"
    
    echo "Uploading kheaders package:"
    ls -lh ${1}

    echo "put \"${1}\" \"kernels/${1}\"" | sftp -oStrictHostKeyChecking=no -oIdentityFile="$keyfile" -b- "${username}\
@${server}"

    if [ $? -ne 0 ] ; then
        rm -f "$keyfile"
        echo "Upload failed. Unable to continue."
        exit 1
    fi

    rm -f "$keyfile"
    echo "Upload succeeded. You could now use --use-remote-package ${1}."

    remote_package="${1}"
}

calc_header_checksums() {
    DEPENDENCY=$(mktemp)
    HEADERS=$(mktemp)
    if [ $? -ne 0 ] ; then
        echo "mktemp failed. Unable to continue."
        rm -f "${HEADERS}" "${DEPENDENCY}"
        return 1
    fi

    for i in "$1/.*.i.cmd"; do
        egrep '.h[[:space:]]?\\$' $i >> $HEADERS
    done

    sort $HEADERS | uniq | sort | \
        sed 's/\\$//g;s/^[[:space:]]*//g;s/^\$//g;s/^(wildcard//g;s/)[[:space:]]*$//g;s/^[[:space:]]*//g' > $DEPENDENCY

    if [ $? -ne 0 ] ; then
        echo "Failed to sort dependency."
        rm -f "${HEADERS}" "${DEPENDENCY}"
        return 1
    fi

    rm -f "${HEADERS}" "$2"
    dirlen=$(readlink -f "${source_dir}" | wc -c)

    while read line; do 
        echo $line | grep ^include > /dev/null 2>&1

        # Common Headers
        if [ $? -ne 0 ]; then
            echo "$line" | grep ^"$(readlink -f "${source_dir}")" > /dev/null 2>&1

            if [ $? -ne 0 ]; then
                continue
            fi

            path_end=$(echo $line | tail -c +$dirlen)
            sum=$(md5sum "${line}" | cut -d ' ' -f 1 2>/dev/null)

            if [ $? -ne 0 ] ; then
                    echo "Failed to get checksum for file: ${linkfile}"
                    echo "Unable to continue."
                    rm -f "${DEPENDENCY}"
                    return 1
            fi

            echo "${sum} source/${path_end}" >> "$2"

        # Generated Headers
        else
            sum=$(md5sum "${kernel}/${line}" | cut -d ' ' -f 1 2>/dev/null)

            if [ $? -ne 0 ] ; then
                echo "Failed to get checksum for file: ${kernel}/${line}"
                echo "Unable to continue."
                rm -f "${DEPENDENCY}"
                return 1
            fi

            echo "${sum} generated/${line}" >> "$2"
        fi
    done < ${DEPENDENCY}

    rm "${DEPENDENCY}"
    return 0
}

gen_header_checksums() {
    if [ -f "${cache_dir}/pkgtmp/dependency_mod/env" ] ; then
        . "${cache_dir}/pkgtmp/dependency_mod/env"
    else
        echo "No build environment found, unable to produce header checksums."
        return 1
    fi

    if [ -n "${CROSS_COMPILE}" ] ; then
        command -v ${CROSS_COMPILE}gcc > /dev/null 2>&1

        if [ $? -ne 0 ] ; then
            echo "${CROSS_COMPILE}gcc not found. Do you have the cross compiler in PATH?"
            return 1
        fi
    fi

    make -C "$kernel" ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE $CUST_KENV M="${cache_dir}/pkgtmp/dependency_mod" depmod.i 2>&1 > /dev/null
    
    if [ $? -ne 0 ] ; then
        echo "Compilation failed. Unable to compute header dependency tree."
        return 1
    fi

    calc_header_checksums "${cache_dir}/pkgtmp/dependency_mod" "$1"
    return $?
}

check_symvers() {
    searchdir="$1"
    symvers="$2"

    searchvers=$(mktemp)
    symvers_parsed=$(mktemp)

    sort $(find "$searchdir" -name \*.mod.c) | uniq | \
        egrep "{ 0x[[:xdigit:]]{8}," | awk '{print $2,$3}' | tr -d ',\"' > $searchvers

    awk '{print $1,$2}' "$symvers" > "$symvers_parsed"

    sort "$symvers_parsed" "$searchvers" | uniq -d | diff "$searchvers" - > /dev/null
    ret=$?

    rm -f "$symvers_parsed" "$searchvers"

    return $ret
}

lookup_cache() {
    if [ ! -f "${kernel}/Module.symvers" ] ; then
        echo "${kernel}/Module.symvers does not exist. Unable to lookup cache."
        return 1
    fi

    ls "${cache_dir}"/*.pkg >/dev/null 2>&1

    if [ $? -ne 0 ] ; then
        echo "Can't find any cache files."
        return 1
    fi

    for pkg in $(ls -t "${cache_dir}"/*.pkg) ; do
        pkg=$(basename "$pkg" .pkg)
        echo -n "Cache lookup: ${pkg}.pkg ... "

        if [ ! -f "${cache_dir}/${pkg}.target" -o $(cat "${cache_dir}/${pkg}.target") != "$target" ] ; then
            echo "miss (different target)"
            continue
        fi

        if [ ! -f "${cache_dir}/${pkg}.md5sum" -o ! -f "${cache_dir}/${pkg}.pkgname" ] ; then
            echo "md5sum or pkgname file missing for ${cache_dir}/${pkg}, unable to validate"
            continue
        fi

        rm -rf "${cache_dir}/pkgtmp"
        mkdir "${cache_dir}/pkgtmp"
        tar xf "${cache_dir}/${pkg}.pkg" --strip-components=1 -C "${cache_dir}/pkgtmp"

        check_symvers "${cache_dir}/pkgtmp" "${kernel}/Module.symvers"
        if [ $? -ne 0 ] ; then
            echo "miss (kernel symbol CRCs differ)"
            rm -rf "${cache_dir}/pkgtmp"
            continue
        fi

        tmpsums=$(mktemp)
        gen_header_checksums "$tmpsums"
        if [ $? -ne 0 ] ; then
            echo "checksum calculation failed"
            rm -rf "${cache_dir}/pkgtmp"
            rm -f "$tmpsums"
            continue
        fi

        diff "$tmpsums" "${cache_dir}/${pkg}.md5sum" > /dev/null
        if [ $? -ne 0 ] ; then
            rm -rf "${cache_dir}/pkgtmp"
            rm -f "$tmpsums"
            echo "miss (header checksums differ)"
            continue
        fi

        rm -f "$tmpsums"
        rm -rf "${cache_dir}/pkgtmp"
        echo "hit!"

        pkgname=$(cat "${cache_dir}/${pkg}.pkgname")
        echo "Copying package to $pkgname"
        cp "${cache_dir}/${pkg}.pkg" "$(pwd)/$pkgname"
        touch "${cache_dir}/${pkg}.pkg"

        return 0
    done

    echo "No cache hits."
    return 1
}

do_remote_build() {
    target_config="$target"
    echo "Starting remote build against target ${target}..."

    if [ "$http_client" = "wget" ] ; then
        reply=$($wget --post-data="terminal=1&filename=${remote_package}&target-config=${target_config}&tags=${tags}&use-cache=${using_cache}&script-version=${script_version}&start-build=1" -O - https://${server})
    else
        reply=$($curl -d terminal=1 -d filename="$remote_package" -d target-config="$target_config" -d tags="$tags" -d use-cache="$using_cache" -d script-version="$script_version" -d start-build=1 https://${server})
    fi

    if [ $? -ne 0 ] ; then
        echo "${http_client} failed. Unable to start build."
        exit 1
    fi
    
    status=$(echo "$reply" | head -n 1)

    if [ "$status" != "OK" ] ; then
        echo "Starting the build failed. Unable to continue."
        echo "The server reported:"
        echo "$status"
        exit 1
    fi
    
    build_id=$(echo "$reply" | head -n 2 | tail -n 1)
    
    echo "Build started, id ${build_id}"
    echo "Polling for completion every 10 seconds..."
    
    statusurl="https://${server}/builds/${build_id}/.status"

    while [ 1 ]
    do
        if [ "$http_client" = "wget" ] ; then
            reply=$($wget -q -O - "$statusurl")
        else
            reply=$($curl_quiet "$statusurl")
        fi
        
        if [ $? -ne 0 ] ; then
            echo "Not finished yet; waiting..."
            sleep 10
            continue
        fi
        
        break
    done
    
    echo "Build finished."
    
    status=$(echo "$reply" | head -n 1)
    
    if [ "$status" != "OK" ] ; then
        echo "Build failed. Cannot download package."
        echo "Tuxera has been notified of this failure."
        exit 1
    fi
    
    filename=$(echo "$reply" | head -2 | tail -1)
    fileurl="https://${server}/builds/${build_id}/${filename}"
    
    echo "Downloading ${filename} ..."
    
    if [ "$http_client" = "wget" ] ; then
        $wget -q -O "$filename" "$fileurl"
    else
        $curl -s -o "$filename" "$fileurl"
    fi
    
    if [ $? -ne 0 ] ; then
        echo "Failed. You can still try to download using the link in the e-mail that was sent."
        exit 1
    fi
    
    echo "Download finished."

    if [ -n "$use_cache" ] ; then
        echo "Updating cache..."

        pkgprefix="$(date +%Y-%m-%d-%H-%M-%S)-$(head -c 8 /dev/urandom | md5sum | head -c 4)"
        cp "$filename" "${cache_dir}/${pkgprefix}.pkg"
        echo "$target" > "${cache_dir}/${pkgprefix}.target"
        echo "$filename" > "${cache_dir}/${pkgprefix}.pkgname"

        mkdir "${cache_dir}/pkgtmp"
        tar xf "${cache_dir}/${pkgprefix}.pkg" --strip-components=1 -C "${cache_dir}/pkgtmp"

        gen_header_checksums "${cache_dir}/${pkgprefix}.md5sum"
        if [ $? -ne 0 ] ; then
            echo "Updating cache failed."
            rm -rf "${cache_dir}/${pkgprefix}.pkg"
            rm -f "${cache_dir}/${pkgprefix}.target"
        fi

        rm -rf "${cache_dir}/pkgtmp"
    fi
}

list_targets() {
    if [ "$http_client" = "wget" ] ; then
        reply=$($wget -O - https://${server}/targets.php)
    else
        reply=$($curl https://${server}/targets.php)
    fi

    if [ $? -ne 0 ] ; then
        echo "Unable to list targets."
        exit 1
    fi

    echo
    echo "Available targets for this user:"
    echo "$reply"
    echo
}

check_http_client() {
    curl_quiet="curl -s -f -u ${username}:${password}"
    wget="wget -nv --user ${username} --password ${password}"
    
    if [ -n "$ignore_certificates" ] ; then
        curl_quiet=${curl_quiet}" -k"
        wget=${wget}" --no-check-certificate"
    fi

    curl=${curl_quiet}" -S"

    if [ -n "$http_client" ] ; then
        echo "HTTP client forced to ${http_client}"
        return
    fi
    
    http_client="curl"
    echo -n "Checking for 'curl'... "
    command -v curl > /dev/null

    if [ $? -ne 0 ] ; then
        echo "no."
        http_client="wget"
        echo -n "Checking for 'wget'... "
        command -v wget > /dev/null
    fi

    if [ $? -ne 0 ] ; then
        echo "no. Unable to continue."
        exit 1
    fi

    echo "yes."
}

check_cmds() {
    echo -n "Checking for: "

    for c in $* ; do
        echo -n "$c "
        command -v $c > /dev/null

        if [ $? -ne 0 ] ; then
            echo "... no."
            echo "Unable to continue."
            exit 1
        fi
    done

    echo "... yes."
    return 0
}

check_autobuild_prerequisites() {
    check_cmds date stty mktemp chmod tail expr head md5sum

    if [ "$http_client" = "wget" ] ; then
        echo -n "Checking for 'sftp'... "

        command -v sftp > /dev/null

        if [ $? -ne 0 ] ; then
            echo "no. Unable to continue."
            exit 1
        fi

        echo "yes."
    fi

    if [ -n "$use_cache" ] ; then
        check_cmds egrep sed awk readlink basename touch uniq sort tr diff

        kernel="$source_dir"

        if [ -n "$output_dir" ] ; then
            kernel="$output_dir"
        fi
    fi
}

echo "tuxera_update.sh version $script_version"

if ! options=$(getopt -u -o pah -l target:,user:,pass:,use-package:,use-remote-package:,source-dir:,output-dir:,version:,cache-dir:,server:,help,ignore-cert,no-check-certificate,use-curl,use-wget,no-excludes,use-cache -- "$@")
then
    usage
fi

set -- $options

while [ $# -gt 0 ]
do
    case $1 in
    -p) pkgonly="yes" ;;
    -a) autobuild="yes" ;;
    --target) target="$2" ; shift;;
    --user) username="$2" ; shift;;
    --pass) password="$2" ; shift;;
    --use-package) local_package="$2" ; shift;;
    --use-remote-package) remote_package="$2" ; shift;;
    --source-dir) source_dir="$2" ; shift;;
    --output-dir) output_dir="$2" ; shift;;
    --ignore-cert) ignore_certificates="yes" ;;
    --no-check-certificate) ignore_certificates="yes" ;;
    --use-wget) http_client="wget" ;;
    --use-curl) http_client="curl" ;;
    --version) tags="$2" ; shift;;
    --no-excludes) no_excludes="yes" ;;
    --use-cache) use_cache="yes" ;;
    --cache-dir) cache_dir="$2" ; shift;;
    --server) server="$2" ; shift;;
    --help | -h) long_help="yes"; usage;;
    (--) shift; break;;
    (-*) echo "$0: error - unrecognized option $1" 1>&2; usage;;
    (*) break;;
    esac
    shift
done

if [ -z "$target" ] ; then
    target="default"
fi

check_cmds tar find date grep

if [ -n "$autobuild" ] ; then
    if [ -n "$pkgonly" ] ; then
        echo "You cannot specify both -p and -a."
        usage
    fi

    if [ -n "$local_package" -a -n "$remote_package" ] ; then
        echo "You cannot specify both local and remote packages."
        usage
    fi

    if [ -n "$local_package" -o -n "$remote_package" ] && [ -n "$use_cache" ] ; then
        echo "You cannot specify --use-package/--use-remote-package with --use-cache."
        usage
    fi

    check_autobuild_prerequisites

    if [ -n "$use_cache" -a "$target" != "list" ] ; then
        mkdir -p "${cache_dir}"
        cache_dir=$(readlink -f "$cache_dir")

        if [ -z "$source_dir" ] ; then
            echo "You must specify kernel source (optionally output) dir to use the cache."
            usage
        fi

        if [ -d "$cache_dir" ] ; then
            lookup_cache

            if [ $? -eq 0 ] ; then
                exit 0
            else
                echo "Proceeding with remote build..."
            fi
        else
            echo "Local cache does not exist; proceeding with remote build..."
        fi
    fi

    if [ -z "$username" ] ; then
        echo -n "Please enter your username: "
        read username
    fi

    if [ -z "$password" ] ; then
        oldstty=$(stty -g)
        echo -n "Please enter your password: "
        stty -echo
        read password
        stty "$oldstty"
        echo
    fi

    check_http_client

    if [ "$target" = "list" ] ; then
        list_targets
        exit 0
    fi

    if [ -z "$local_package" -a -z "$remote_package" ] ; then
        local_package="kheaders_$(date +%Y-%m-%d-%H-%M-%S-$(head -c 8 /dev/urandom | md5sum | head -c 4)).tar.bz2"
        build_package "$source_dir" "$output_dir" "$local_package"
    fi

    if [ -z "$remote_package" ] ; then
        if [ "$http_client" = "wget" ] ; then
            upload_package_sftp "$local_package"
        else
            upload_package_curl "$local_package"
        fi
    fi

    using_cache=$use_cache
    if [ -z "$use_cache" ] ; then
        using_cache="no"
    fi

    do_remote_build "$remote_package"

else
    build_package "$source_dir" "$output_dir" "kheaders.tar.bz2"
fi
    
exit 0
