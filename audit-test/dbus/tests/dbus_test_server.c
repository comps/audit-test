/* Copyright (c) 2014 Red Hat, Inc. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 2 the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This is a D-Bus test server for audit-test suite tests. It exports
 * connection com.redhat.cctest with one object / and these interfaces
 * and respective methods:
 *
 * Interface                                Method(s)
 * org.freedesktop.DBus.Introspectable      Introspect
 * com.redhat.cctest                        Exit, Test
 *
 * Method descriptions:
 * org.freedesktop.DBus.Introspectable.Introspect - returns introspect xml
 * com.redhat.cctest.Exit - exits the listening loop
 * com.redhat.cctest.Test - prints a test message
 *
 * The program does not daemonize and should be run in background for testing.
 *
 */

#include <dbus/dbus.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

/* constants */
#define DBUS_CONN_NAME "com.redhat.cctest"

/* globals */
DBusError dbus_err;
DBusConnection *dbus_conn;

 /* Reply to default interface Introspect from org.freedesktop.DBus */
int dbus_reply_introspect(DBusMessage* msg, DBusConnection* conn)
{
  DBusMessage    *reply;
  DBusMessageIter  args;

  /* add signal listing generation */
  char      *reply_text =
"<!DOCTYPE node PUBLIC \"- /* freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
"   \"http: /* www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
"<node name=\"/org/fedoraproject/ds/dbusplugin\">\n"
"   <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
"     <method name=\"Introspect\">\n"
"       <arg type=\"s\" name=\"xml_data\" direction=\"out\"/>\n"
"     </method>\n"
"   </interface>\n"
"   <interface name=\"org.fedoraproject.ds.dbusplugin\">\n"
"     <method name=\"Exit\"/>\n"
"     <method name=\"Test\"/>\n"
"   </interface>\n</node>";

  /* unique number to associate replies with requests */
  dbus_uint32_t  serial = 0;

  /* read the arguments */
  dbus_message_iter_init(msg, &args);

  /* create a reply from the message */
  reply = dbus_message_new_method_return(msg);

  /* add the arguments to the reply */
  dbus_message_iter_init_append(reply, &args);
  if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &reply_text)) {
    errx(1, "Could not append reply text");
  }

  /* send the reply & flush the connection */
  if (!dbus_connection_send(dbus_conn, reply, &serial)) {
    errx(1, "Could not append reply text");
  }
  dbus_connection_flush(dbus_conn);

  /* free the reply */
  dbus_message_unref(reply);

  return 0;
}

 /* Reply to default interface Introspect from org.freedesktop.DBus */
int dbus_reply_text(DBusMessage* msg, const char* reply_text)
{
  DBusMessageIter  args;
  DBusMessage    *reply;
  int flush = 1;

  /* unique number to associate replies with requests */
  dbus_uint32_t  serial = 0;

  /* read the arguments */
  dbus_message_iter_init(msg, &args);

  /* create a reply from the message */
  reply = dbus_message_new_method_return(msg);

  /* add the arguments to the reply */
  dbus_message_iter_init_append(reply, &args);
  if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING,
    &reply_text)) {
    warnx("Error creating default reply.");
    flush = 0;
  }

  /* send the reply & flush the connection */
  if (!dbus_connection_send(dbus_conn, reply, &serial)) {
    warnx("Error sending default reply.");
    flush = 0;
  }

  if(flush) dbus_connection_flush(dbus_conn);

  /* free the reply */
  dbus_message_unref(reply);

  return 0;
}

/*
 * Create thread that implements the introspect method
 */
void dbus_listener()
{
  DBusMessage    *msg;
  const char *member;

  /* loop, testing for new messages */
  while (1) {
    /* non blocking read of the next available message */
    dbus_connection_read_write(dbus_conn, 1000);
    msg = dbus_connection_pop_message(dbus_conn);

    /* loop again if we haven't got a message */
    if (msg == NULL) {
      continue;
    }

    member = dbus_message_get_member(msg);
    if (member == NULL) {
      continue;
    }

    warnx("Got message: to=%s from=%s member=%s\n",
      dbus_message_get_destination(msg), dbus_message_get_sender(msg), member);

    /* default interface Introspect  */
    if (dbus_message_is_method_call(msg, "org.freedesktop.DBus.Introspectable", "Introspect"))
    {
      warnx("Introspect\n");
      dbus_reply_introspect(msg, dbus_conn);
    }
    /* List all observed operations */
    else if (dbus_message_is_method_call(msg, DBUS_CONN_NAME, "Exit")) {
      dbus_reply_text(msg, "Exiting");
      dbus_message_unref(msg);
      exit(0);
    }
    else if (dbus_message_is_method_call(msg, DBUS_CONN_NAME, "Test")) {
      dbus_reply_text(msg, "This is a message from Test method");
    } else {
      dbus_reply_text(msg, "Unknown method call. Call Exit or Test method.");
    }

    /* free the message */
    dbus_message_unref(msg);
  }
}


int main(int argc, char **argv)
{
  /* initialise the errors */
	dbus_error_init(&dbus_err);

  /* connect to the bus */
	dbus_conn = dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_err);
	if (dbus_error_is_set(&dbus_err)) {
		warn("D-Bus connection Error (%s)\n", dbus_err.message);
		dbus_error_free(&dbus_err);
  exit(1);
	}

	if (dbus_conn == NULL) {
		errx(1, "D-Bus connection failed.\n");
	}

  /* Initialize new unique dbus name */
  char *signal = DBUS_CONN_NAME;
  int ret = dbus_bus_request_name(dbus_conn, signal,
  DBUS_NAME_FLAG_DO_NOT_QUEUE, &dbus_err);

  if (dbus_error_is_set(&dbus_err)) {
  warnx("Error requesting bus name: %s\n", dbus_err.message);
  dbus_error_free(&dbus_err);
  exit(1);
  }

  if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
  errx(1, "Unable to became primary owner of the %s bus name\n", DBUS_CONN_NAME);
  }

  dbus_listener();

  return 0;
}

/* vim: set ts=2 sw=2 tw=2 et : */
