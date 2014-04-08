###############################################################################
#   Copyright (c) 2014 Red Hat, Inc. All rights reserved.
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of version 2 the GNU General Public License as
#   published by the Free Software Foundation.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
###############################################################################

source testcase.bash || exit 2

#
# internal function helpers
#
# these functions try hard to minimize the usage of local variables
# to avoid name conflicts (when using eval $1=...) as much as possible
#

# parses $1 as "user:group" into separate user ($2) and group ($3)
# in the current shell
_parse_owner()
{
    local IFS=':'
    # $1 = input, $2 = user, $3 = group
    read "$2" "$3" <<<"$1"
    return 0
}

# parses $1 in the "subject_spec" format (run.conf) and fills in
# user:group and supplementary groups to variable names
# specified as $2 and $3 (respectively) in the current shell,
# all arguments **must** always be specified
#   ie. _parse_subj $subject_spec user group other_groups
_parse_subj()
{
    local IFS=','
    # $1 = input, $2 = user:group, $3 = suppl_groups
    read "$2" "$3" <<<"$1"
    return 0
}

# parses $1 in the "object_spec" format (run.conf) and fills in
# object type, owning user:group, file mode (permissions),
# posix acl_specs and text representation of capabilities
# to variable names specified as $2, $3, $4 and $5 respectively,
# in the current shell,
# all arguments **must** always be specified
#   ie. _parse_obj $object_spec obj_type obj_owner obj_perms obj_acl
_parse_obj()
{
    local IFS='#'
    # $1 = input, $2 = type, $3 = user:group, $4 = mode, $5 = acls
    read "$2" "$3" "$4" "$5" <<<"$1"
    return 0
}


#
# functions useful for DAC testing,
# for specific argument names refer to run.conf for this bucket
#
# ALL functions:
# - must take care of cleanup for their respective actions,
#   ie. by adding content using append_cleanup
# - must never return if an error occured, must call exit_error
# - should return 0 if the operation was successful
#

# ensure that a given user/group exists, create it if not,
# ensure that supplementary groups exist, ereate them if not
#
# usage: create_subj <subject_spec>
create_subj()
{
    local owner= user= group= suppl_groups=
    _parse_subj "$1" owner suppl_groups
    _parse_owner "$owner" user group

    # sanity
    [ -z "$user" -o -z "$group" ] && \
        exit_error "$FUNCNAME: subject_spec arg invalid, user or group empty"

    # check/create supplementary groups
    local OLDIFS="$IFS" g=
    IFS=','
    for g in $suppl_groups; do
        if ! getent group "$g" >/dev/null; then
            prepend_cleanup "groupdel \"$g\" &>/dev/null"
            groupadd "$g" || exit_error
        fi
    done;
    IFS="$OLDIFS"

    # create group, if it doesn't exist already
    if ! getent group "$group" >/dev/null; then
        prepend_cleanup "groupdel \"$group\" &>/dev/null"
        groupadd "$group" || exit_error
    fi

    # create the user, if it doesn't exist already
    # - adding the user to the group or providing supplementary groups
    #   is not strictly necessary for this use case as run_as() simply
    #   sets the user/groups from scratch, regardless of passwd or group
    #   database associations
    if ! getent passwd "$user" >/dev/null; then
        prepend_cleanup "userdel -rf \"$user\""
        if [ "$suppl_groups" ]; then
            useradd -g "$group" -G "$suppl_groups" "$user" || exit_error
        else
            useradd -g "$group" "$user" || exit_error
        fi
    fi

    return 0
}

# run command in a specified environment and return its return value
#
# usage: run_as -s <subject_spec> [-c <file_cap>] <cmd> [args]
#
# - specifying empty argument to an option is the same as never specifying
#   the option at all
run_as()
{
    local OPTARG= OPTIND= arg=
    local file_cap= cmd_file=
    local owner= user= group= suppl_groups=
    local uid= gid= suppl_gids=

    # POSIXLY_CORRECT honored by default (set -o posix)
    while getopts "s:c:" arg; do
        case "$arg" in
            s)
                _parse_subj "$OPTARG" owner suppl_groups
                _parse_owner "$owner" user group
                ;;
            c)
                file_cap="$OPTARG"
                ;;
        esac
    done;
    shift "$((OPTIND-1))"

    # sanity
    [ -z "$user" -o -z "$group" ] && \
        exit_error "$FUNCNAME: subject_spec arg invalid, user or group empty"
    [ "$#" -lt 1 -o -z "$1" ] && \
        exit_error "$FUNCNAME: command not specified / empty"
    cmd_file=$(which $1) || \
        exit_error "$FUNCNAME: command doesn't exist or is not in PATH"

    # check whether user, primary group and all supplementary groups exist
    if ! getent passwd "$user" >/dev/null; then
        exit_error "$FUNCNAME: user $user doesn't exist"
    fi
    if ! getent group "$group" >/dev/null; then
        exit_error "$FUNCNAME: group $group doesn't exist"
    fi
    local OLDIFS="$IFS" g=
    IFS=','
    for g in $suppl_groups; do
        if ! getent group "$g" >/dev/null; then
            exit_error "$FUNCNAME: supplementary group $g doesn't exist"
        fi
    done;
    IFS="$OLDIFS"

    # TODO: drop capabilities support here

    # set file capabilities on the executable binary
    if [ "$file_cap" ]; then
        backup "$cmd_file"
        setcap "$file_cap" "$cmd_file"
    fi

    run-as.py -u "$user" -g "$group" -G "$suppl_groups" "$@"

    return $?
}

# create an object with specified properties on the filesystem
# - if path was specified, create the object on that path
# - if path was unspecified, choose an appropriate location
#   and store the path of the created object in OBJ_PATH
#
# if the owning user/group doesn't exist, create it
#
# usage: create_obj <object_spec> [path]
create_obj()
{
    local spec="$1" path="$2"
    local type= owner= user= group= mode= acls=
    local mqbase="/dev/mqueue"

    # parse the object spec
    _parse_obj "$spec" type owner mode acls
    _parse_owner "$owner" user group

    # sanity
    [ -z "$user" -o -z "$group" ] && \
        exit_error "$FUNCNAME: object_spec arg invalid, user or group empty"
    [ "$#" -gt 2 ] && \
        exit_error "$FUNCNAME: too much arguments passed"

    # error if it already exists (unexpected)
    [ "$path" -a -e "$path" ] && \
        exit_error "$FUNCNAME: $path already exists"

    # create the object owner, if it doesn't exist
    if ! getent group "$group" >/dev/null; then
        prepend_cleanup "groupdel \"$group\" &>/dev/null"
        groupadd "$group" || exit_error
    fi
    if ! getent passwd "$user" >/dev/null; then
        prepend_cleanup "userdel -rf \"$user\""
        useradd -g "$group" "$user" || exit_error
    fi

    # create the path
    # - don't override $path !
    # - define OBJ_PATH only if $path is unspecified (empty)
    case "$type" in

        file)
            local file="$path"
            if [ "$file" ]; then
                touch "$file" || exit_error
            else
                # /tmp is noexec, use / as TMPDIR
                file=$(mktemp -p /) || exit_error
            fi
            append_cleanup "rm -f \"$file\""
            # owner
            chown "$user:$group" "$file" || exit_error
            # mode
            chmod "$mode" "$file" || exit_error
            # acls
            # - setfacl supports comma-separated specs
            if [ "$acls" ]; then
                setfacl -m "$acls" "$file" || exit_error
            fi
            [ -z "$path" ] && declare -g OBJ_PATH="$file"
            ;;

        dir)
            local dir="$path"
            if [ "$dir" ]; then
                mkdir "$dir" || exit_error
            else
                # /tmp is noexec, use / as TMPDIR
                dir=$(mktemp -p / -d) || exit_error
            fi
            append_cleanup "rm -rf \"$dir\""
            # owner
            chown "$user:$group" "$dir" || exit_error
            # mode
            chmod "$mode" "$dir" || exit_error
            # acls
            # - setfacl supports comma-separated specs
            if [ "$acls" ]; then
                setfacl -m "$acls" "$dir" || exit_error
            fi
            [ -z "$path" ] && declare -g OBJ_PATH="$dir"
            ;;

        mq)
            # ignore if $path set
            local mqueue="$(mktemp -u -p /)"
            do_mq_open $mqueue create || \
                exit_error "$FUNCNAME: could not create mqueue $mqueue"
            append_cleanup "rm -f \"$mqbase$mqueue\""
            chown "$user:$group" "$mqbase$mqueue" || \
                exit_error "$FUNCNAME: could not chown mqueue"
            chmod "$mode" "$mqbase$mqueue" || \
                exit_error "$FUNCNAME: could not chmod mqueue"
            declare -g OBJ_PATH="$mqueue"
            ;;

        mqdir)
            # do not create directory, just set atributes
            # on $mqbase, reset in cleanup
            # does not export $OBJ_PATH
            chown "$user:$group" "$mqbase" || \
                exit_error "$FUNCNAME: could not chown $mqbase"
            chmod "$mode" "$mqbase" || \
                exit_error "$FUNCNAME: could not chmod $mqbase"
            append_cleanup "chown root:root $mqbase; chmod 1777 $mqbase"
            ;;

        *)
            exit_error "$FUNCNAME: unsupported object type: $type"
    esac

    return 0
}

set -x

# vim: sts=4 sw=4 et :
