#!/bin/bash -x
###############################################################################
#   Copyright (c) 2014 Red Hat, Inc. All rights reserved.
#
#   This copyrighted material is made available to anyone wishing
#   to use, modify, copy, or redistribute it subject to the terms
#   and conditions of the GNU General Public License version 2.
#
#   This program is distributed in the hope that it will be
#   useful, but WITHOUT ANY WARRANTY; without even the implied
#   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#   PURPOSE. See the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public
#   License along with this program; if not, write to the Free
#   Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
#   Boston, MA 02110-1301, USA.
###############################################################################
#
# AUTHOR: Ondrej Moris <omoris@redhat.com>
#
# DESCRIPTION: Basic polkit sanity testing.
#
# We test the enforcement of a polkit policy for a custom testing binary
# (/bin/whoami) by setting allow_any parameter of a policy. The testing is
# done by executing the testing binary via pkexec as a non-privileged user.
# We check that the policy settings are applied correctly and that the binary
# is actually executed by the privileged user (ie. /bin/whoami reports 'root').

source testcase.bash || exit 2

# Polkit testing action ID.
POLKIT_A_ID="org.freedesktop.policykit.cctest"

# Testing user.
POLKIT_USER=testuser

# Binary on which polkit policy will be tested.
POLKIT_PATH=/bin/whoami

# Testing polkit policy.
POLKIT_CONF=/usr/share/polkit-1/actions/$POLKIT_A_ID.policy

# Create testing policy.
function policy_setup {

    local allow_any=$1

    [ -z "$allow_any" ] && exit_error "Missing allow_any argument"

    cat >$POLKIT_CONF<<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE policyconfig PUBLIC "-//freedesktop//DTD PolicyKit Policy Configuration 1.0//EN"
        "http://www.freedesktop.org/standards/PolicyKit/1/policyconfig.dtd">
<policyconfig>
  <action id="$POLKIT_A_ID">
    <description>Testing policy</description>
    <defaults>
      <allow_any>$allow_any</allow_any>
    </defaults>
    <annotate key="org.freedesktop.policykit.exec.path">$POLKIT_PATH</annotate>
  </action>
</policyconfig>
EOF

    chmod a+r $POLKIT_CONF
}

# Wrapper for pkexec "expect"-ed calls. According to given policy,
# out testing binary is executed with root privileges:
#   a)   immediately (yes),
#   b)   never (no),
#   c-d) after successful testing user authorization (auth_self
#        and auth_self_keep),
#   e-f) after successful testing admin authorization (auth_admin
#        and auth_admin_keep).
function pkexec_wrapper {

    local allow_policy=$1

    [ -z "$allow_policy" ] && exit_error "Missing policy argument"

    testadmin_wheel_order=$(getent group wheel | awk -F ':' '{print $4}' | \
        tr ',' '\n' | grep testadmin -n | awk -F ':' '{print $1}')

    case $allow_policy in
        "yes")
            expect -c "
              spawn runuser --user $POLKIT_USER pkexec $POLKIT_PATH;
              expect {root} {wait; exit 0}
              exit 1" || exit_fail "Incorrect result for 'yes'"
            ;;
        "no")
            expect -c "
              spawn runuser --user $POLKIT_USER pkexec $POLKIT_PATH;
              expect {
                {root} {exit 2}
                {Not authorized} {wait; exit 0}
              }
              exit 1" || exit_fail "Incorrect result for 'no'"
            ;;
        "auth_self")
            expect -c "
              spawn runuser --user $POLKIT_USER pkexec $POLKIT_PATH;
              expect {
                {root} {exit 1}
                {$POLKIT_A_ID} {
                  expect {Authenticating as: testuser} {
                    expect {Password:} {
                      send \"$TEST_USER_PASSWD\r\";
                      expect {AUTHENTICATION COMPLETE} {
                        expect {root} {wait; exit 0}
                      }
                    }
                  }
                }
              }
              exit 1;" || exit_fail "Incorrect result for 'auth_self'"
            ;;
        "auth_self_keep")
            expect -c "
              spawn runuser --user $POLKIT_USER /bin/bash
              expect -re \"[testuser.*\$\" {
                send \"pkexec $POLKIT_PATH\r\"
                expect {
                  {root} {exit 1}
                  {$POLKIT_A_ID} {
                    expect {Authenticating as: testuser} {
                      expect {Password:} {
                        send \"$TEST_USER_PASSWD\r\";
                        expect {AUTHENTICATION COMPLETE} {
                          expect {root} {
                            expect -re \"[testuser.*\$\" {
                              send \"sleep 5\r\"
                              send \"pkexec $POLKIT_PATH\r\"
                                expect {root} {send \"logout\r\"; exit 0}
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
              exit 1;" || exit_fail "Incorrect result for 'auth_self_keep'"
            ;;
        "auth_admin")
            expect -c "
              spawn runuser --user $POLKIT_USER pkexec $POLKIT_PATH;
              expect {
                {root} {exit 1}
                {$POLKIT_A_ID} {
                  expect {Choose identity to authenticate as} {
                    send \"3\r\";
                    expect {Password:} {
                      send \"$TEST_ADMIN_PASSWD\r\";
                      expect {AUTHENTICATION COMPLETE} {
                        expect {root} {wait; exit 0}
                      }
                    }
                  }
                }
              }
              exit 1;" || exit_fail "Incorrect result for 'auth_admin'"
            ;;
        "auth_admin_keep")
            expect -c "
              spawn runuser --user $POLKIT_USER /bin/bash
              expect -re \"[testuser.*\$\" {
                send \"pkexec $POLKIT_PATH\r\"
                expect {
                  {root} {exit 1}
                  {$POLKIT_A_ID} {
                    expect {Choose identity to authenticate as} {
                      send \"3\r\";
                      expect {Password:} {
                        send \"$TEST_ADMIN_PASSWD\r\";
                        expect {AUTHENTICATION COMPLETE} {
                          expect {root} {
                            expect -re \"[testuser.*\$\" {
                              send \"sleep 5\r\"
                              send \"pkexec $POLKIT_PATH\r\"
                                expect {root} {send \"logout\r\"; exit 0}
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
              exit 1;" || exit_fail "Incorrect result for 'auth_admin_keep'"
            ;;
        "*")
            exit_error "Incorrect policy"
            ;;
    esac

    return 0
}

# Test for allow_any policy parameter correctness.
function test_pkexec_allow {

    append_cleanup "rm -f $POLKIT_CONF"

    for allow_any in "yes" "no" "auth_self" "auth_admin" \
        "auth_self_keep" "auth_admin_keep"; do

        policy_setup "$allow_any"

        # Wait for polkitd to "reload" the policy.
        sleep 5

        pkexec_wrapper "$allow_any"
    done

    return 0
}

# Test.
if [ "$(type -t test_$1)" = "function" ]; then
    eval test_$1
else
    help
fi

exit_pass
