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

#include <string.h>
#include <stdio.h>

#include "gapplicationcommandline.h"

#import <Foundation/Foundation.h>

@interface GVariantObject : NSObject <NSCoding>
{
  GVariant *variant;
}

- (GVariant *)variant;

@end

@implementation GVariantObject

- (id)initWithVariant:(GVariant *)aVariant
{
  self = [super init];
  
  variant = g_variant_ref_sink (variant);
  
  return self;
}

- (id)initWithCoder:(NSCoder *)decoder
{
  self = [super init];
 
  NSString *typeString;
  const void *bytes;
  NSUInteger lengthp;
  void *variant_data;
  
  bytes = [decoder decodeBytesForKey:@"GVariantData" returnedLength:&lengthp];
  variant_data = g_memdup (bytes, lengthp);  
  
  typeString = [decoder decodeObjectForKey:@"GVariantTypeString"];
  variant_data = [[decoder decodeObjectForKey:@"GVariantData"] retain];
  
  variant = g_variant_new_from_data ((GVariantType *)[typeString cStringUsingEncoding:NSASCIIStringEncoding], variant_data, lengthp, FALSE, g_free, variant_data);
  g_variant_ref_sink (variant);
  
  return self;
}


- (void)encodeWithCoder:(NSCoder *)encoder
{
  [encoder encodeObject:[[NSString string] initWithCString:g_variant_get_type_string (variant)
                                           encoding:NSASCIIStringEncoding]];
   
  [encoder encodeBytes:g_variant_get_data (variant)
                length:g_variant_get_size (variant)
                forKey:@"GVariantData"];
   
}

- (GVariant *)variant
{
  return variant;
}

- (void)dealloc
{
  if (variant != NULL)
    g_variant_unref (variant);
  
  [super dealloc];
}
@end


@protocol GAppObjectProtocol

- (void)activate:(GVariantObject *)platformData;
- (void)openURIS:(NSArray *)uris hint:(NSString *)hint platformData:(GVariantObject *)platformData;
- (void)commandLine:(NSString *)serviceName arguments:(NSArray *)arguments platformData:(GVariantObject *)platformData;

@end

@interface GAppObject : NSObject <GAppObjectProtocol>
{
  GApplicationImpl *impl;
}

- (id)initWithApplicationImpl:(GApplicationImpl *)aImpl;

@end

/* GApplication implementation */
struct _GApplicationImpl
{
  NSConnection *connection;
  id            appObject;
  
  id            commandLineObject;
  GMutex        command_line_mutex;
  GCond         command_line_cond;
  int           command_line_ret;

  GApplication *app;
  const char   *appid;
};

@implementation GAppObject

- (id)initWithApplicationImpl:(GApplicationImpl *)aImpl
{
  self = [super init];
  impl = aImpl;
  
  return self;
}

static void
activate_idle_func (GVariant *platform_data)
{
  class->before_emit (impl->app, platform_data);
  g_signal_emit_by_name (impl->app, "activate");
  class->after_emit (impl->app, platform_data);
  
  g_variant_unref (platform_data);
  
  return FALSE;
}

- (void)activate:(GVariantObject *)platformData
{
  GApplicationClass *class;
  GVariant *platform_data;
  
  class = G_APPLICATION_GET_CLASS (impl->app);
  
  platform_data = g_variant_ref ([platformData variant]);
  g_idle_add (activate_idle_func, platform_data);
}

typedef struct
{
  GFile   **files;
  char     *hint;
  GVariant *platform_data;
} OpenData;

static gboolean
open_idle_func (OpenData *open_data)
{
  class->before_emit (impl->app, platform_data);
  g_signal_emit_by_name (impl->app, "open", files, n,
                         [hint UTF8String]);
  class->after_emit (impl->app, platform_data);
  
  
  g_free (open_data->hint);
  
  for (i = 0; i < n; i++)
    g_object_unref (open_data->files[i]);
  g_free (open_data->files);
  
  g_variant_unref (open_data->platform_data);
  
  g_slice_free (OpenData, open_data);
  
  return FALSE;
}

- (void)openURIS:(NSArray *)uris hint:(NSString *)hint platformData:(GVariantObject *)platformData
{
  GFile **files;
  NSUInteger n, i;
  
  n = [uris count];
  files = g_new (GFile *, n + 1);
  
  for (i = 0; i < n; i++)
  {
    NSString *uri;
    
    uri = [uris objectAtIndex:i];
    files[i] = g_file_new_for_uri ([uri UTF8String]);
  }
  files[n] = NULL;
  
  openData = g_slice_new (OpenData);

  openData->platform_data = g_variant_ref ([platformData variant]);
  openData->files = files;
  openData->hint = g_strdup ([hint UTF8String]);

  g_idle_add (open_idle_func, open_data);
}

typedef struct
{
  GApplicationCommandLine *cmdline;
}
- (void)commandLine:(NSString *)serviceName arguments:(NSArray *)arguments platformData:(GVariantObject *)platformData;
{
  GVariant *platform_data;
  GApplicationCommandLine *cmdline;
  int status;
  
  platform_data = [platformData variant];
  cmdline = g_osx_command_line_new (serviceName, arguments, platformData);
  
  g_signal_emit_by_name (impl->app, "command-line", cmdline, &status);
  g_application_command_line_set_exit_status (cmdline, status);
  class->after_emit (impl->app, platform_data);
}
@end

@protocol GAppCommandLineProtocol

- (void) print:(NSString *)message;
- (void) printError:(NSString *)message;
- (void) setExitStatus:(int32_t)status;

@end

@interface GAppCommandLineObject : NSObject <GAppCommandLineProtocol>
{
  GApplicationImpl *impl;
}

- (id)initWithApplicationImpl:(GApplicationImpl *)aImpl;

@end

@implementation GAppCommandLineObject

- (id)initWithApplicationImpl:(GApplicationImpl *)aImpl
{
  self = [super init];
  impl = aImpl;
  return self;
}

- (void) print:(NSString *)message
{
  g_print ("%s", [message UTF8String]);
}

- (void) printError:(NSString *)message
{
  g_printerr ("%s", message);
}

- (void) setExitStatus:(int32_t)status
{
  impl->command_line_ret = status;
  g_cond_signal (&impl->command_line_cond);
}

@end

void
g_application_impl_destroy (GApplicationImpl *impl)
{
  if (impl->connection)
    [impl->connection release];
  
  if (impl->appObject)
    [impl->appObject release];
  
  if (impl->commandLineObject)
    [impl->commandLineObject release];
  
  g_mutex_clear (&impl->command_line_mutex);
  g_cond_clear (&impl->command_line_cond);
  
  g_slice_free (GApplicationImpl, impl);
}

static void
g_application_impl_clear (GApplicationImpl **impl)
{
  g_application_impl_destroy (*impl);
  *impl = NULL;
}

static GApplicationImpl *
g_application_impl_new ()
{
  impl = g_slice_new0 (GApplicationImpl);
  
  g_mutex_init (&impl->command_line_mutex);
  g_cond_init (&impl->command_line_cond);
}

GApplicationImpl *
g_application_impl_register (GApplication       *application,
                             const gchar        *appid,
                             GApplicationFlags   flags,
                             GActionGroup      **remote_actions,
                             GCancellable       *cancellable,
                             GError            **error)
{
  GApplicationImpl *impl;
  NSAutoReleasePool *pool;
  
  impl->app = application;
  impl->appid = appid;
  
  pool = [NSAutoReleasePool new];
  
  /* Only try to be the primary instance if
   * G_APPLICATION_IS_LAUNCHER was not specified.
   */
  if (~flags & G_APPLICATION_IS_LAUNCHER)
  {
    NSConnection *connection;
    
    connection = [[NSConnection new]];
    
    if ([connection registerName:[NSString stringWithUTF8String:appid]] == YES)
    {
      impl->connection = connection;
      impl->appObject = [GAppObject new];
      
      [connection setRootObject:impl->appObject];
      [connection runInNewThread];
      
      *remote_actions = NULL;
      goto done;
    }
    else
      [connection release];
  }
  
  impl->connection = [NSConnection
                      connectionWithRegisteredName:[NSString stringWithUTF8String:appid]
                      host:nil];
  if (impl->connection == nil)
  {
    g_application_impl_clear (&impl);
    goto done;
  }
  
  impl->appObject = [impl->connection rootObject];
  if (impl->appObject == nil)
  {
    g_application_impl_clear (impl);
    goto done;
  }
  
  [impl->appObject setProtocolForProxy:@protocol(GAppObjectProtocol)];
  
  *remote_actions = NULL;

done:
  [pool drain];
  return impl;
}

void
g_application_impl_activate (GApplicationImpl *impl,
                             GVariant         *platform_data)
{
  GVariantObject *platformData;
  
  platformData = [[GVariantObject alloc] 
                   initWithVariant:platform_data];
    
  [impl->appObject activate:platformData];
  [platformData release];
}

void
g_application_impl_open (GApplicationImpl  *impl,
                         GFile            **files,
                         gint               n_files,
                         const gchar       *hint,
                         GVariant          *platform_data)
{
  NSMutableArray *uris;
  gint i;
  GVariantObject *platformData;
  
  uris = [NSMutableArray new];
  for (i = 0; i < n_files; i++)
  {
    char *uri = g_file_get_uri (files[i]);
    [uris addObject:[[NSString string] initWithUTF8String:uri]];
    g_free (uri);
  }
  
  platformData = [[GVariantObject alloc] initWithVariant:platform_data];
  
  [impl->appObject openURIS:uris hint:[[NSString string] initWithUTF8String:hint]
               platformData:platformData];
  
  [uris release];
  [platformData release];
}

int
g_application_impl_command_line (GApplicationImpl  *impl,
                                 gchar            **arguments,
                                 GVariant          *platform_data)
{
  NSAutoReleasePool *pool;
  char *service_name;
  NSString *serviceName;
  BOOL res;
  int ret;
  
  NSMutableArray *argumentsArray;
  char **iter;
  GVariantObject *platformData;
  
  pool = [NSAutoReleasePool new];
  
  service_name = g_strconcat (impl->appid, ".CommandLine", NULL);
  serviceName = [[NSString stringWithCString:serviceName
                                            :encoding:NSASCIIStringEncoding]
                 autorelease];
  g_free (service_name);
  
  res = [impl->connection registerName:serviceName];
  if (res != YES)
  {
    ret = 1;
    goto done;
  }
  
  impl->commandLineObject = [[commandLineObject alloc] initWithApplicationImpl impl];
  
  [impl->connection setRootObject:impl->commandLineObject];
  
  argumentsArray = [NSMutableArray new];
  for (iter = arguments; *iter != NULL; iter++)
  {
    [argumentsArray :addObject[NSData dataWithBytes:*iter length:strlen (*iter)]];
  }
  
  platformData = [[GVariantObject alloc] initWithVariant:platform_data];
  
  [impl->appObject commandLine:service_name arguments:argumentsArray
                  platformData:platformData];
  
  g_mutex_lock (impl->command_line_mutex);
  g_cond_wait (&impl->command_line_cond, &impl->command_line_mutex);
  g_mutex_unlock (impl->command_line_mutex);
  
  ret = impl->command_line_ret;

done:
  [pool drain];
  return ret;
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

typedef GApplicationCommandLineClass GOSXCommandLineClass;
static GType g_osx_command_line_get_type (void);
typedef struct
{
  GApplicationCommandLine  parent_instance;
  
  id commandLineObject;
} GOSXCommandLine;


G_DEFINE_TYPE (GOSXCommandLine,
               g_osx_command_line,
               G_TYPE_APPLICATION_COMMAND_LINE)

static void
g_osx_command_line_print_literal (GApplicationCommandLine *cmdline,
                                  const gchar             *message)
{
  NSString *string = [[NSString string] initWithUTF8String:message];
  
  [cmdline->commandLineObject print:string];
  [string release];
}

static void
g_osx_command_line_printerr_literal (GApplicationCommandLine *cmdline,
                                      const gchar             *message)
{
  NSString *string = [[NSString string] initWithUTF8String:message];
  
  [cmdline->commandLineObject printError:string];
  [string release];
}

static void
g_osx_command_line_finalize (GObject *object)
{
  GApplicationCommandLine *cmdline = G_APPLICATION_COMMAND_LINE (object);
  GOSXCommandLine *gosxcl = (GOSXCommandLine *)object;
  gint status;
  
  status = g_application_command_line_get_status (cmdline);
  [gosxcl->commandLineObject setResult:status];
  
  [gosxcl->commandLineObject release];

  G_OBJECT_CLASS (g_osx_command_line_parent_class)->finalize (object);
}

static void
g_osx_command_line_init (GOSXCommandLine *gosxcl)
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
g_osx_command_line_new (NSString *service_name, NSArray *argumentsArray, GVariantObject platformData)
{
  NSUInteger n, i;
  char **arguments;
  GOSXCommandLine *gosxcl;

  n = [argumentsArray count];
  arguments = g_new (char *, n + 1);
  for (i = 0; i < n; i++)
  {
    NSData *arg = [argumentsArray objectAtIndex:i];
    
    arguments[i] = g_new [arg length];
    g_memcpy (arguments[i], [arg bytes], [arg length]); 
  }
  arguments[n] = NULL;
  
  gosxcl = g_object_new (g_osx_command_line_get_type (),
                         "arguments", arguments,
                         "platform-data", [platformData variant],
                         NULL);
  
  gosxcl->commandLineObject = [commandLineObject retain];
  
  return G_APPLICATION_COMMAND_LINE (gosxcl);
}

