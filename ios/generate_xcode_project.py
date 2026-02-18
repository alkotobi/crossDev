#!/usr/bin/env python3
"""
Generate an Xcode project for iOS
This script creates a proper .xcodeproj file structure
"""

import os
import uuid
import json
from pathlib import Path

def generate_uuid():
    """Generate a UUID-like string for Xcode project"""
    return uuid.uuid4().hex.upper()[:24]

# Project configuration
PROJECT_NAME = "NativeWindow"
PROJECT_DIR = Path(__file__).parent
ROOT_DIR = PROJECT_DIR.parent
XCODE_PROJECT_DIR = PROJECT_DIR / f"{PROJECT_NAME}.xcodeproj"
PROJECT_FILE = XCODE_PROJECT_DIR / "project.pbxproj"

# Source files
CORE_SOURCES = [
    "src/main.cpp",
    "src/window.cpp",
    "src/webview.cpp",
    "src/button.cpp",
    "src/input_field.cpp",
    "src/application.cpp",
    "src/event_handler.cpp",
    "src/message_router.cpp",
    "src/handlers/create_window_handler.cpp",
    "src/handlers/app_info_handler.cpp",
    "src/handlers/calculator_handler.cpp",
    "src/handlers/file_dialog_handler.cpp",
]

IOS_SOURCES = [
    "src/platform/ios/AppDelegate.mm",
    "src/platform/ios/app_ios.mm",
    "src/platform/ios/window_ios.mm",
    "src/platform/ios/webview_ios.mm",
    "src/platform/ios/button_ios.mm",
    "src/platform/ios/filedialog_ios.mm",
    "src/platform/ios/input_ios.mm",
]

HEADER_FILES = [
    "include/window.h",
    "include/webview.h",
    "include/button.h",
    "include/input_field.h",
    "include/application.h",
    "include/event_handler.h",
    "include/message_router.h",
    "include/message_handler.h",
    "include/platform.h",
    "include/handlers/create_window_handler.h",
    "include/handlers/app_info_handler.h",
    "include/handlers/calculator_handler.h",
    "include/handlers/file_dialog_handler.h",
    "include/nlohmann/json.hpp",
    "include/nlohmann/json_fwd.hpp",
]

def generate_project():
    """Generate the Xcode project file"""
    
    # Create project directory
    XCODE_PROJECT_DIR.mkdir(exist_ok=True)
    
    # Generate UUIDs for all objects
    project_uuid = generate_uuid()
    main_group_uuid = generate_uuid()
    sources_group_uuid = generate_uuid()
    headers_group_uuid = generate_uuid()
    ios_group_uuid = generate_uuid()
    handlers_group_uuid = generate_uuid()
    target_uuid = generate_uuid()
    build_config_list_uuid = generate_uuid()
    debug_config_uuid = generate_uuid()
    release_config_uuid = generate_uuid()
    
    # Generate file references
    file_refs = {}
    build_files = []
    file_refs_uuid = {}
    
    all_sources = CORE_SOURCES + IOS_SOURCES
    
    for file_path in all_sources + HEADER_FILES:
        file_uuid = generate_uuid()
        file_refs_uuid[file_path] = file_uuid
        file_refs[file_uuid] = {
            "isa": "PBXFileReference",
            "lastKnownFileType": get_file_type(file_path),
            "path": file_path,
            "sourceTree": "<group>"
        }
    
    # Create build phases
    sources_build_phase_uuid = generate_uuid()
    frameworks_build_phase_uuid = generate_uuid()
    
    # Generate project content
    project_content = f'''// !$*UTF8*$!
{{
	archiveVersion = 1;
	classes = {{
	}};
	objectVersion = 56;
	objects = {{

/* Begin PBXBuildFile section */
'''
    
    # Add build files for sources
    for file_path in all_sources:
        file_uuid = file_refs_uuid[file_path]
        build_file_uuid = generate_uuid()
        build_files.append((build_file_uuid, file_uuid))
        project_content += f'''		{build_file_uuid} /* {os.path.basename(file_path)} in Sources */ = {{isa = PBXBuildFile; fileRef = {file_uuid} /* {os.path.basename(file_path)} */; }};
'''
    
    # Add frameworks - need separate UUIDs for build file and file reference
    framework_refs = {}
    framework_file_refs = {}
    for framework in ["UIKit", "Foundation", "WebKit"]:
        framework_build_uuid = generate_uuid()
        framework_file_uuid = generate_uuid()
        framework_refs[framework] = framework_build_uuid
        framework_file_refs[framework] = framework_file_uuid
        project_content += f'''		{framework_build_uuid} /* {framework}.framework in Frameworks */ = {{isa = PBXBuildFile; fileRef = {framework_file_uuid} /* {framework}.framework */; }};
'''
    
    project_content += '''/* End PBXBuildFile section */

/* Begin PBXFileReference section */
'''
    
    # Add file references
    for file_path, file_uuid in file_refs_uuid.items():
        file_type = get_file_type(file_path)
        project_content += f'''		{file_uuid} /* {os.path.basename(file_path)} */ = {{isa = PBXFileReference; lastKnownFileType = {file_type}; path = "{file_path}"; sourceTree = "<group>"; }};
'''
    
    # Add Info.plist
    info_plist_uuid = generate_uuid()
    project_content += f'''		{info_plist_uuid} /* Info.plist */ = {{isa = PBXFileReference; lastKnownFileType = text.plist.xml; path = Info.plist; sourceTree = "<group>"; }};
'''
    
    # Add executable
    product_uuid = generate_uuid()
    project_content += f'''		{product_uuid} /* {PROJECT_NAME}.app */ = {{isa = PBXFileReference; explicitFileType = wrapper.application; includeInIndex = 0; path = "{PROJECT_NAME}.app"; sourceTree = BUILT_PRODUCTS_DIR; }};
'''
    
    # Add framework file references (using the UUIDs we already generated)
    for framework in ["UIKit", "Foundation", "WebKit"]:
        framework_file_uuid = framework_file_refs[framework]
        project_content += f'''		{framework_file_uuid} /* {framework}.framework */ = {{isa = PBXFileReference; lastKnownFileType = wrapper.framework; name = {framework}.framework; path = System/Library/Frameworks/{framework}.framework; sourceTree = SDKROOT; }};
'''
    
    project_content += '''/* End PBXFileReference section */

/* Begin PBXFrameworksBuildPhase section */
'''
    
    frameworks_phase_uuid = generate_uuid()
    project_content += f'''		{frameworks_phase_uuid} /* Frameworks */ = {{
			isa = PBXFrameworksBuildPhase;
			buildActionMask = 2147483647;
			files = (
'''
    
    for framework, build_file_uuid in framework_refs.items():
        project_content += f'''				{build_file_uuid} /* {framework}.framework in Frameworks */,
'''
    
    project_content += '''			);
			runOnlyForDeploymentPostprocessing = 0;
		};
/* End PBXFrameworksBuildPhase section */

/* Begin PBXGroup section */
'''
    
    # Main group structure
    # Products group needs separate UUID from product file reference
    products_group_uuid = generate_uuid()
    
    project_content += "		" + main_group_uuid + " = {\n"
    project_content += '''			isa = PBXGroup;
			children = (
				''' + sources_group_uuid + ''' /* Sources */,
				''' + headers_group_uuid + ''' /* Headers */,
				''' + info_plist_uuid + ''' /* Info.plist */,
				''' + products_group_uuid + ''' /* Products */,
			);
			sourceTree = "<group>";
		};
		''' + products_group_uuid + ''' /* Products */ = {
			isa = PBXGroup;
			children = (
				''' + product_uuid + ''' /* ''' + PROJECT_NAME + '''.app */,
			);
			name = Products;
			sourceTree = "<group>";
		};
		{sources_group_uuid} /* Sources */ = {{
			isa = PBXGroup;
			children = (
				{ios_group_uuid} /* ios */,
				{handlers_group_uuid} /* handlers */,
'''
    
    # Add core source files
    for file_path in CORE_SOURCES:
        if "handlers" not in file_path:
            file_uuid = file_refs_uuid[file_path]
            project_content += f'''				{file_uuid} /* {os.path.basename(file_path)} */,
'''
    
    project_content += f'''			);
			path = Sources;
			sourceTree = "<group>";
		}};
		{ios_group_uuid} /* ios */ = {{
			isa = PBXGroup;
			children = (
'''
    
    # Add iOS source files
    for file_path in IOS_SOURCES:
        file_uuid = file_refs_uuid[file_path]
        project_content += f'''				{file_uuid} /* {os.path.basename(file_path)} */,
'''
    
    project_content += f'''			);
			path = ios;
			sourceTree = "<group>";
		}};
		{handlers_group_uuid} /* handlers */ = {{
			isa = PBXGroup;
			children = (
'''
    
    # Add handler files
    for file_path in CORE_SOURCES:
        if "handlers" in file_path:
            file_uuid = file_refs_uuid[file_path]
            project_content += f'''				{file_uuid} /* {os.path.basename(file_path)} */,
'''
    
    project_content += f'''			);
			path = handlers;
			sourceTree = "<group>";
		}};
		{headers_group_uuid} /* Headers */ = {{
			isa = PBXGroup;
			children = (
'''
    
    # Add header files
    for file_path in HEADER_FILES:
        file_uuid = file_refs_uuid[file_path]
        project_content += f'''				{file_uuid} /* {os.path.basename(file_path)} */,
'''
    
    project_content += '''			);
			path = Headers;
			sourceTree = "<group>";
		};
/* End PBXGroup section */

/* Begin PBXNativeTarget section */
'''
    
    # Native target
    project_content += f'''		{target_uuid} /* {PROJECT_NAME} */ = {{
			isa = PBXNativeTarget;
			buildConfigurationList = {build_config_list_uuid} /* Build configuration list for PBXNativeTarget "{PROJECT_NAME}" */;
			buildPhases = (
				{generate_uuid()} /* Sources */,
				{frameworks_phase_uuid} /* Frameworks */,
				{generate_uuid()} /* Resources */,
			);
			buildRules = (
			);
			dependencies = (
			);
			name = {PROJECT_NAME};
			productName = {PROJECT_NAME};
			productReference = {product_uuid} /* {PROJECT_NAME}.app */;
			productType = "com.apple.product-type.application";
		}};
/* End PBXNativeTarget section */

/* Begin PBXProject section */
'''
    
    # Generate project config list UUID before using it
    project_config_list_uuid = generate_uuid()
    
    project_content += "		" + project_uuid + " /* Project object */ = {\n"
    project_content += '''			isa = PBXProject;
			attributes = {
				BuildIndependentTargetsInParallel = 1;
				LastSwiftUpdateCheck = 1500;
				LastUpgradeCheck = 1500;
				TargetAttributes = {
					''' + target_uuid + ''' = {
						CreatedOnToolsVersion = 15.0;
					};
				};
			};
			buildConfigurationList = ''' + project_config_list_uuid + ''' /* Build configuration list for PBXProject "''' + PROJECT_NAME + '''" */;
			compatibilityVersion = "Xcode 14.0";
			developmentRegion = en;
			hasScannedForEncodings = 0;
			knownRegions = (
				en,
				Base,
			);
			mainGroup = {main_group_uuid};
			productRefGroup = ''' + products_group_uuid + ''' /* Products */;
			projectDirPath = "";
			projectRoot = "";
			targets = (
				{target_uuid} /* {PROJECT_NAME} */,
			);
		}};
/* End PBXProject section */

/* Begin PBXSourcesBuildPhase section */
		{generate_uuid()} /* Sources */ = {{
			isa = PBXSourcesBuildPhase;
			buildActionMask = 2147483647;
			files = (
'''
    
    # Add source files to build phase
    for build_file_uuid, file_uuid in build_files:
        file_path = [k for k, v in file_refs_uuid.items() if v == file_uuid][0]
        project_content += f'''				{build_file_uuid} /* {os.path.basename(file_path)} in Sources */,
'''
    
    project_content += '''			);
			runOnlyForDeploymentPostprocessing = 0;
		};
/* End PBXSourcesBuildPhase section */

/* Begin XCBuildConfiguration section */
'''
    
    # Debug configuration
    project_content += "		" + debug_config_uuid + " /* Debug */ = {\n"
    project_content += '''			isa = XCBuildConfiguration;
			buildSettings = {
				ALWAYS_SEARCH_USER_PATHS = NO;
				ASSETCATALOG_COMPILER_APPICON_NAME = AppIcon;
				CLANG_ANALYZER_NONNULL = YES;
				CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION = YES_AGGRESSIVE;
				CLANG_CXX_LANGUAGE_STANDARD = "c++17";
				CLANG_ENABLE_MODULES = YES;
				CLANG_ENABLE_OBJC_ARC = YES;
				CLANG_ENABLE_OBJC_WEAK = YES;
				CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING = YES;
				CLANG_WARN_BOOL_CONVERSION = YES;
				CLANG_WARN_COMMA = YES;
				CLANG_WARN_CONSTANT_CONVERSION = YES;
				CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS = YES;
				CLANG_WARN_DIRECT_OBJC_ISA_USAGE = YES_ERROR;
				CLANG_WARN_DOCUMENTATION_COMMENTS = YES;
				CLANG_WARN_EMPTY_BODY = YES;
				CLANG_WARN_ENUM_CONVERSION = YES;
				CLANG_WARN_INFINITE_RECURSION = YES;
				CLANG_WARN_INT_CONVERSION = YES;
				CLANG_WARN_NON_LITERAL_NULL_CONVERSION = YES;
				CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF = YES;
				CLANG_WARN_OBJC_LITERAL_CONVERSION = YES;
				CLANG_WARN_OBJC_ROOT_CLASS = YES_ERROR;
				CLANG_WARN_QUOTED_INCLUDE_IN_FRAMEWORK_HEADER = YES;
				CLANG_WARN_RANGE_LOOP_ANALYSIS = YES;
				CLANG_WARN_STRICT_PROTOTYPES = YES;
				CLANG_WARN_SUSPICIOUS_MOVE = YES;
				CLANG_WARN_UNGUARDED_AVAILABILITY = YES_AGGRESSIVE;
				CLANG_WARN_UNREACHABLE_CODE = YES;
				CLANG_WARN__DUPLICATE_METHOD_MATCH = YES;
				CODE_SIGN_STYLE = Automatic;
				"CODE_SIGN_IDENTITY[sdk=iphoneos*]" = "iPhone Developer";
				CURRENT_PROJECT_VERSION = 1;
				DEVELOPMENT_TEAM = "";
				ENABLE_BITCODE = NO;
				ENABLE_STRICT_OBJC_MSGSEND = YES;
				GCC_C_LANGUAGE_STANDARD = gnu11;
				GCC_DYNAMIC_NO_PIC = NO;
				GCC_NO_COMMON_BLOCKS = YES;
				GCC_OPTIMIZATION_LEVEL = 0;
				GCC_PREPROCESSOR_DEFINITIONS = (
					"DEBUG=1",
					"$(inherited)",
				);
				GCC_WARN_64_TO_32_BIT_CONVERSION = YES;
				GCC_WARN_ABOUT_RETURN_TYPE = YES_ERROR;
				GCC_WARN_UNDECLARED_SELECTOR = YES;
				GCC_WARN_UNINITIALIZED_AUTOS = YES_AGGRESSIVE;
				GCC_WARN_UNUSED_FUNCTION = YES;
				GCC_WARN_UNUSED_VARIABLE = YES;
				HEADER_SEARCH_PATHS = (
					"$(SRCROOT)/../include",
					"$(inherited)",
				);
				INFOPLIST_FILE = Info.plist;
				INFOPLIST_KEY_UIApplicationSupportsIndirectInputEvents = YES;
				INFOPLIST_KEY_UILaunchScreen = {{}};
				INFOPLIST_KEY_UISupportedInterfaceOrientations = "UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
				INFOPLIST_KEY_UISupportedInterfaceOrientations~ipad = "UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
				IPHONEOS_DEPLOYMENT_TARGET = 12.0;
				LD_RUNPATH_SEARCH_PATHS = (
					"$(inherited)",
					"@executable_path/Frameworks",
				);
				MTL_ENABLE_DEBUG_INFO = INCLUDE_SOURCE;
				MTL_FAST_MATH = YES;
				ONLY_ACTIVE_ARCH = YES;
				OTHER_CPLUSPLUSFLAGS = (
					"$(OTHER_CFLAGS)",
					"-std=c++17",
					"-fobjc-arc",
				);
				PRODUCT_BUNDLE_IDENTIFIER = "com.nativewindow.NativeWindow";
				PRODUCT_NAME = "$(TARGET_NAME)";
				SDKROOT = iphoneos;
				TARGETED_DEVICE_FAMILY = "1,2";
			};
			name = Debug;
		};
'''
    
    # Release configuration
    project_content += "		" + release_config_uuid + " /* Release */ = {\n"
    project_content += '''			isa = XCBuildConfiguration;
			buildSettings = {
			isa = XCBuildConfiguration;
			buildSettings = {{
				ALWAYS_SEARCH_USER_PATHS = NO;
				ASSETCATALOG_COMPILER_APPICON_NAME = AppIcon;
				CLANG_ANALYZER_NONNULL = YES;
				CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION = YES_AGGRESSIVE;
				CLANG_CXX_LANGUAGE_STANDARD = "c++17";
				CLANG_ENABLE_MODULES = YES;
				CLANG_ENABLE_OBJC_ARC = YES;
				CLANG_ENABLE_OBJC_WEAK = YES;
				CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING = YES;
				CLANG_WARN_BOOL_CONVERSION = YES;
				CLANG_WARN_COMMA = YES;
				CLANG_WARN_CONSTANT_CONVERSION = YES;
				CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS = YES;
				CLANG_WARN_DIRECT_OBJC_ISA_USAGE = YES_ERROR;
				CLANG_WARN_DOCUMENTATION_COMMENTS = YES;
				CLANG_WARN_EMPTY_BODY = YES;
				CLANG_WARN_ENUM_CONVERSION = YES;
				CLANG_WARN_INFINITE_RECURSION = YES;
				CLANG_WARN_INT_CONVERSION = YES;
				CLANG_WARN_NON_LITERAL_NULL_CONVERSION = YES;
				CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF = YES;
				CLANG_WARN_OBJC_LITERAL_CONVERSION = YES;
				CLANG_WARN_OBJC_ROOT_CLASS = YES_ERROR;
				CLANG_WARN_QUOTED_INCLUDE_IN_FRAMEWORK_HEADER = YES;
				CLANG_WARN_RANGE_LOOP_ANALYSIS = YES;
				CLANG_WARN_STRICT_PROTOTYPES = YES;
				CLANG_WARN_SUSPICIOUS_MOVE = YES;
				CLANG_WARN_UNGUARDED_AVAILABILITY = YES_AGGRESSIVE;
				CLANG_WARN_UNREACHABLE_CODE = YES;
				CLANG_WARN__DUPLICATE_METHOD_MATCH = YES;
				CODE_SIGN_STYLE = Automatic;
				"CODE_SIGN_IDENTITY[sdk=iphoneos*]" = "iPhone Developer";
				CURRENT_PROJECT_VERSION = 1;
				DEVELOPMENT_TEAM = "";
				ENABLE_BITCODE = NO;
				ENABLE_NS_ASSERTIONS = NO;
				ENABLE_STRICT_OBJC_MSGSEND = YES;
				GCC_C_LANGUAGE_STANDARD = gnu11;
				GCC_NO_COMMON_BLOCKS = YES;
				GCC_WARN_64_TO_32_BIT_CONVERSION = YES;
				GCC_WARN_ABOUT_RETURN_TYPE = YES_ERROR;
				GCC_WARN_UNDECLARED_SELECTOR = YES;
				GCC_WARN_UNINITIALIZED_AUTOS = YES_AGGRESSIVE;
				GCC_WARN_UNUSED_FUNCTION = YES;
				GCC_WARN_UNUSED_VARIABLE = YES;
				HEADER_SEARCH_PATHS = (
					"$(SRCROOT)/../include",
					"$(inherited)",
				);
				INFOPLIST_FILE = Info.plist;
				INFOPLIST_KEY_UIApplicationSupportsIndirectInputEvents = YES;
				INFOPLIST_KEY_UILaunchScreen = {{}};
				INFOPLIST_KEY_UISupportedInterfaceOrientations = "UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
				INFOPLIST_KEY_UISupportedInterfaceOrientations~ipad = "UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight";
				IPHONEOS_DEPLOYMENT_TARGET = 12.0;
				LD_RUNPATH_SEARCH_PATHS = (
					"$(inherited)",
					"@executable_path/Frameworks",
				);
				MTL_ENABLE_DEBUG_INFO = NO;
				MTL_FAST_MATH = YES;
				OTHER_CPLUSPLUSFLAGS = (
					"$(OTHER_CFLAGS)",
					"-std=c++17",
					"-fobjc-arc",
				);
				PRODUCT_BUNDLE_IDENTIFIER = "com.nativewindow.NativeWindow";
				PRODUCT_NAME = "$(TARGET_NAME)";
				SDKROOT = iphoneos;
				TARGETED_DEVICE_FAMILY = "1,2";
				VALIDATE_PRODUCT = YES;
			};
			name = Release;
		};
/* End XCBuildConfiguration section */

/* Begin XCConfigurationList section */
'''
    
    # Build configuration list for target
    project_config_list_uuid = generate_uuid()
    project_content += "		" + build_config_list_uuid + " /* Build configuration list for PBXNativeTarget \"" + PROJECT_NAME + "\" */ = {\n"
    project_content += '''			isa = XCConfigurationList;
			buildConfigurations = (
				''' + debug_config_uuid + ''' /* Debug */,
				''' + release_config_uuid + ''' /* Release */,
			);
			defaultConfigurationIsVisible = 0;
			defaultConfigurationName = Release;
		};
		''' + project_config_list_uuid + ''' /* Build configuration list for PBXProject "''' + PROJECT_NAME + '''" */ = {
			isa = XCConfigurationList;
			buildConfigurations = (
				''' + debug_config_uuid + ''' /* Debug */,
				''' + release_config_uuid + ''' /* Release */,
			);
			defaultConfigurationIsVisible = 0;
			defaultConfigurationName = Release;
		};
/* End XCConfigurationList section */
	};
	rootObject = ''' + project_uuid + ''' /* Project object */;
}
'''
    
    # Write project file
    with open(PROJECT_FILE, 'w') as f:
        f.write(project_content)
    
    print(f"✅ Xcode project created at: {XCODE_PROJECT_DIR}")
    print(f"   Project file: {PROJECT_FILE}")
    print("\n📝 Next steps:")
    print("   1. Open the project in Xcode:")
    print(f"      open {XCODE_PROJECT_DIR}")
    print("   2. Select a development team in Signing & Capabilities")
    print("   3. Build and run on iOS Simulator or device")

def get_file_type(file_path):
    """Get Xcode file type from file extension"""
    ext = os.path.splitext(file_path)[1].lower()
    type_map = {
        '.cpp': 'sourcecode.cpp.cpp',
        '.mm': 'sourcecode.cpp.objcpp',
        '.h': 'sourcecode.c.h',
        '.hpp': 'sourcecode.cpp.h',
        '.plist': 'text.plist.xml',
    }
    return type_map.get(ext, 'text')

if __name__ == "__main__":
    generate_project()
