#!/bin/bash

set -o errexit
set -o xtrace

user="darkk"
proj="redsocks"
versions="0.1 0.2 0.3 0.4"

for file in `python -c "import urllib2, json; print '\n'.join(d['name'] for d in json.load(urllib2.urlopen('https://api.github.com/repos/${user}/${proj}/downloads')))"`; do
    touch "$file.uploaded"
done

token=""

for ver in $versions; do
    for pkg in tar.gz tar.bz2 tar.lzma; do
        case $pkg in
        tar.gz)   sortof=gzip ;;
        tar.bz2)  sortof=bzip2 ;;
        tar.lzma) sortof=lzma ;;
        esac

        tag="release-${ver}"
        gittar="github-$tag.tar.gz"
        gooddir="${proj}-${ver}"
        file="${proj}-${ver}.${pkg}"

        if [ -r "${file}.uploaded" ]; then
            continue
        fi

        rm -rf "${user}-${proj}-"*

        if [ ! -r "$file" ]; then
            if [ ! -d "$gooddir" ]; then
                if [ ! -f "$gittar" ]; then
                    wget -O "$gittar" "https://github.com/${user}/${proj}/tarball/${tag}"
                fi
                tar --extract --file "$gittar"
                mv "${user}-${proj}-"* "$gooddir"
            fi
            tar "--${sortof}" --create --file "$file" "$gooddir"
        fi

        if [ -z "$token" ]; then
            if [ ! -r git-repack.sh.token ]; then
                curl --data '{"scopes": ["public_repo"], "note": "uploader script"}' -u "$user" "https://api.github.com/authorizations" \
                    | python -c "import sys, json, pipes; print 'token=%s' % pipes.quote(json.load(sys.stdin)['token'])" \
                    > git-repack.sh.token
            fi
            . git-repack.sh.token
        fi

        . <(python -c "import os, urllib2, json, pipes; print '\n'.join('GITHUB_%s=%s' % (k.replace('-', '_'), pipes.quote(str(v))) for k, v in json.load(urllib2.urlopen('https://api.github.com/repos/${user}/${proj}/downloads?access_token=${token}', data=json.dumps({'name': '${file}', 'size': os.stat('${file}').st_size, 'description': 'Version ${ver}, ${sortof}', 'content-type': 'application/x-gtar-compressed'}))).iteritems())")
        curl \
            --include \
            -F "key=${GITHUB_path}" -F "acl=${GITHUB_acl}" -F "success_action_status=201" \
            -F "Filename=${GITHUB_name}" \
            -F "AWSAccessKeyId=${GITHUB_accesskeyid}" -F "Policy=${GITHUB_policy}" -F "Signature=${GITHUB_signature}" \
            -F "Content-Type=${GITHUB_mime_type}" \
            -F "file=@${file}" https://github.s3.amazonaws.com/
        echo;
        touch "${file}.uploaded"
    done
done
