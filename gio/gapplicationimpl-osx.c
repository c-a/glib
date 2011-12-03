/*
 * Copyright © 2011 Carl-Anton Ingmarsson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors: Carl-Anton Ingmarsson <ca.ingmarsson@gmail.com>
 */

#include "gapplicationimpl.h"

#include "gactiongroup.h"
#include "gapplication.h"
#include "gfile.h"
#include "gdbusconnection.h"
#include "gdbusintrospection.h"
#include "gdbuserror.h"

#include <string.h>
#include <stdio.h>

#include "gapplicationcommandline.h"

/* GApplication implementation */
struct _GApplicationImpl
{
};

static gchar *
application_path_from_appid (const gchar *appid)
{
  gchar *appid_path, *iter;
  
  appid_path = g_strconcat ("/", appid, NULL);
  for (iter = appid_path; *iter; iter++)
  {
    if (*iter == '.')
      *iter = '/';
    
    if (*iter == '-')
      *iter = '_';
  }
  
  return appid_path;
}

void
g_application_impl_destroy (GApplicationImpl *impl)
{
}

GApplicationImpl *
g_application_impl_register (GApplication       *application,
                             const gchar        *appid,
                             GApplicationFlags   flags,
                             GHashTable        **remote_actions,
                             GCancellable       *cancellable,
                             GError            **error)
{
}

void
g_application_impl_activate (GApplicationImpl *impl,
                             GVariant         *platform_data)
{
}

void
g_application_impl_open (GApplicationImpl  *impl,
                         GFile            **files,
                         gint               n_files,
                         const gchar       *hint,
                         GVariant          *platform_data)
{
}

static void
g_application_impl_cmdline_method_call (GDBusConnection       *connection,
                                        const gchar           *sender,
                                        const gchar           *object_path,
                                        const gchar           *interface_name,
                                        const gchar           *method_name,
                                        GVariant              *parameters,
                                        GDBusMethodInvocation *invocation,
                                        gpointer               user_data)
{
}

static void
g_application_impl_cmdline_done (GObject      *source,
                                 GAsyncResult *result,
                                 gpointer      user_data)
{
}

int
g_application_impl_command_line (GApplicationImpl  *impl,
                                 gchar            **arguments,
                                 GVariant          *platform_data)
{
}

void
g_application_impl_change_action_state (GApplicationImpl *impl,
                                        const gchar      *action_name,
                                        GVariant         *value,
                                        GVariant         *platform_data)
{
}

void
g_application_impl_activate_action (GApplicationImpl *impl,
                                    const gchar      *action_name,
                                    GVariant         *parameter,
                                    GVariant         *platform_data)
{
}

void
g_application_impl_flush (GApplicationImpl *impl)
{
}


/* GOSXCommandLine implementation */

typedef GOSXCommandLineClass GOSXCommandLineClass;
static GType g_osx_command_line_get_type (void);
typedef struct
{
  GApplicationCommandLine  parent_instance;
} GOSXCommandLine;


G_DEFINE_TYPE (GOSXCommandLine,
               g_osx_command_line,
               G_TYPE_APPLICATION_COMMAND_LINE)

static void
g_osx_command_line_print_literal (GApplicationCommandLine *cmdline,
                                  const gchar             *message)
{
}

static void
g_osx_command_line_printerr_literal (GApplicationCommandLine *cmdline,
                                      const gchar             *message)
{
}

static void
g_osx_command_line_finalize (GObject *object)
{  
  G_OBJECT_CLASS (g_osx_command_line_parent_class)->finalize (object);
}

static void
g_osx_command_line_init (GDBusCommandLine *gdbcl)
{
}

static void
g_osx_command_line_class_init (GApplicationCommandLineClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  
  object_class->finalize = g_osx_command_line_finalize;
  class->printerr_literal = g_osx_command_line_printerr_literal;
  class->print_literal = g_osx_command_line_print_literal;
}

static GApplicationCommandLine *
g_osx_command_line_new ()
{
  GOSXCommandLine *gosxcl;

  gosxcl = g_object_new (g_osx_command_line_get_type (), NULL);
  
  return G_APPLICATION_COMMAND_LINE (gosxcl);
}

