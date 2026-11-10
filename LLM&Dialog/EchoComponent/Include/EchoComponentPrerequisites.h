
#ifndef __EchoComponentPrerequisites_H__
#define __EchoComponentPrerequisites_H__
 

#include "EchoEnginePrerequisites.h"

namespace Echo
{
  	class ScriptManager;

	class EntityComponent;
	class PhysicsPage;
	class PhysicsPagedLayer;
	class EchoPhysicsSystem;
	typedef EchoWeakPtr<EntityComponent> EntityComponentPtr;
}

// Windows Settings
#if ECHO_PLATFORM == ECHO_PLATFORM_WIN32

// If we're not including this from a client build, specify that the stuff
// should get exported. Otherwise, import it.
#	if defined( ECHO_STATIC_LIB )
// Linux compilers don't have symbol import/export directives.
#   	define _EchoComponentExport
#   else
#   	if defined( ECHOCOMPONENT_EXPORTS )
#       	define _EchoComponentExport __declspec( dllexport )
#   	else
#           if defined( __MINGW32__ )
#               define _EchoComponentExport
#           else
#       	    define _EchoComponentExport __declspec( dllimport )
#           endif
#   	endif
#	endif

#endif // ECHO_PLATFORM == ECHO_PLATFORM_WIN32

//----------------------------------------------------------------------------
// Linux/Apple/iOs/Android/NaCl Settings
#if ECHO_PLATFORM == ECHO_PLATFORM_LINUX || ECHO_PLATFORM == ECHO_PLATFORM_APPLE || ECHO_PLATFORM == ECHO_PLATFORM_APPLE_IOS || \
    ECHO_PLATFORM == ECHO_PLATFORM_ANDROID || ECHO_PLATFORM == ECHO_PLATFORM_NACL || ECHO_PLATFORM == ECHO_PLATFORM_OHOS

// Enable GCC symbol visibility
#   if defined( ECHO_GCC_VISIBILITY )
#       define _EchoComponentExport  __attribute__ ((visibility("default")))
#   else
#       define _EchoComponentExport
#   endif

#endif

#endif 


