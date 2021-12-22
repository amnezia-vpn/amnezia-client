# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

require 'xcodeproj'

class XCodeprojPatcher
  attr :project
  attr :target_main
  attr :target_extension

  def run(file, shortVersion, fullVersion, platform, networkExtension, configHash, adjust_sdk_token)
    open_project file
    open_target_main

    die 'IOS requires networkExtension mode' if not networkExtension and platform == 'ios'

    group = @project.main_group.new_group('Configuration')
    @configFile = group.new_file('xcode.xconfig')

    setup_target_main shortVersion, fullVersion, platform, networkExtension, configHash, adjust_sdk_token

#    if platform == 'macos'
#      setup_target_loginitem shortVersion, fullVersion, configHash
#      setup_target_nativemessaging shortVersion, fullVersion, configHash
#    end


    if networkExtension
      setup_target_extension shortVersion, fullVersion, platform, configHash
      setup_target_gobridge
    else
      setup_target_wireguardgo
      setup_target_wireguardtools
    end

    setup_target_balrog if platform == 'macos'

    @project.save
  end

  def open_project(file)
    @project = Xcodeproj::Project.open(file)
    die 'Failed to open the project file: ' + file if @project.nil?
  end

  def open_target_main
    @target_main = @project.targets.find { |target| target.to_s == 'AmneziaVPN' }
    return @target_main if not @target_main.nil?

    die 'Unable to open AmneziaVPN target'
  end

  def setup_target_main(shortVersion, fullVersion, platform, networkExtension, configHash, adjust_sdk_token)
    @target_main.build_configurations.each do |config|
      config.base_configuration_reference = @configFile

      config.build_settings['LD_RUNPATH_SEARCH_PATHS'] ||= '"$(inherited) @executable_path/../Frameworks"'
      config.build_settings['SWIFT_VERSION'] ||= '5.0'
      config.build_settings['CLANG_ENABLE_MODULES'] ||= 'YES'
      config.build_settings['SWIFT_OBJC_BRIDGING_HEADER'] ||= 'macos/app/WireGuard-Bridging-Header.h'
      config.build_settings['FRAMEWORK_SEARCH_PATHS'] ||= [
        "$(inherited)",
        "$(PROJECT_DIR)/3rd",
        "$(PROJECT_DIR)/3rd/OpenVPNAdapter/build/Debug-iphoneos"
      ]

      # Versions and names
      config.build_settings['MARKETING_VERSION'] ||= shortVersion
      config.build_settings['CURRENT_PROJECT_VERSION'] ||= fullVersion
      config.build_settings['PRODUCT_BUNDLE_IDENTIFIER'] = configHash['APP_ID_MACOS'] if platform == 'macos'
      config.build_settings['PRODUCT_BUNDLE_IDENTIFIER'] = configHash['APP_ID_IOS'] if platform == 'ios'
      config.build_settings['PRODUCT_NAME'] = 'AmneziaVPN'

      # other config
      config.build_settings['INFOPLIST_FILE'] ||= platform + '/app/Info.plist'
      if platform == 'ios'
        config.build_settings['CODE_SIGN_ENTITLEMENTS'] ||= 'ios/app/main.entitlements'
        if adjust_sdk_token != ""
          config.build_settings['ADJUST_SDK_TOKEN'] = adjust_sdk_token
        end
      elsif networkExtension
        config.build_settings['CODE_SIGN_ENTITLEMENTS'] ||= 'macos/app/app.entitlements'
      else
        config.build_settings['CODE_SIGN_ENTITLEMENTS'] ||= 'macos/app/daemon.entitlements'
      end

      config.build_settings['CODE_SIGN_IDENTITY'] ||= 'Apple Development'
      config.build_settings['ENABLE_BITCODE'] ||= 'NO' if platform == 'ios'
      config.build_settings['SDKROOT'] = 'iphoneos' if platform == 'ios'
      config.build_settings['SWIFT_PRECOMPILE_BRIDGING_HEADER'] = 'NO' if platform == 'ios'

      groupId = "";
      if (platform == 'macos')
        groupId = configHash['DEVELOPMENT_TEAM'] + "." + configHash['GROUP_ID_MACOS']
        config.build_settings['APP_ID_MACOS'] ||= configHash['APP_ID_MACOS']
      else
        groupId = configHash['GROUP_ID_IOS']
        config.build_settings['GROUP_ID_IOS'] ||= configHash['GROUP_ID_IOS']
        # Force xcode to not set QT_LIBRARY_SUFFIX to "_debug", which causes crash
        config.build_settings['QT_LIBRARY_SUFFIX'] = ""
      end

      config.build_settings['GCC_PREPROCESSOR_DEFINITIONS'] ||= [
        'GROUP_ID=\"' + groupId + '\"',
        "VPN_NE_BUNDLEID=\\\"" + (platform == 'macos' ? configHash['NETEXT_ID_MACOS'] : configHash['NETEXT_ID_IOS']) + "\\\"",
      ]

      if config.name == 'Release'
        config.build_settings['SWIFT_OPTIMIZATION_LEVEL'] ||= '-Onone'
      end
    end
    
#   images_ref = @target_main.new_reference('ios/app/Images.xcassets')
#   @target_main.add_resources([images_ref])

    if networkExtension
      # WireGuard group
      group = @project.main_group.new_group('WireGuard')

      [
        'macos/gobridge/wireguard-go-version.h',
        '3rd/wireguard-apple/Sources/Shared/Keychain.swift',
        '3rd/wireguard-apple/Sources/WireGuardKit/IPAddressRange.swift',
        '3rd/wireguard-apple/Sources/WireGuardKit/InterfaceConfiguration.swift',
        '3rd/wireguard-apple/Sources/Shared/Model/NETunnelProviderProtocol+Extension.swift',
        '3rd/wireguard-apple/Sources/WireGuardKit/TunnelConfiguration.swift',
        '3rd/wireguard-apple/Sources/Shared/Model/TunnelConfiguration+WgQuickConfig.swift',
        '3rd/wireguard-apple/Sources/WireGuardKit/Endpoint.swift',
        '3rd/wireguard-apple/Sources/Shared/Model/String+ArrayConversion.swift',
        '3rd/wireguard-apple/Sources/WireGuardKit/PeerConfiguration.swift',
        '3rd/wireguard-apple/Sources/WireGuardKit/DNSServer.swift',
        '3rd/wireguard-apple/Sources/WireGuardApp/LocalizationHelper.swift',
        '3rd/wireguard-apple/Sources/Shared/FileManager+Extension.swift',
        '3rd/wireguard-apple/Sources/WireGuardKitC/x25519.c',
        '3rd/wireguard-apple/Sources/WireGuardKit/PrivateKey.swift',
      ].each { |filename|
        file = group.new_file(filename)
        @target_main.add_file_references([file])
      }

      # @target_main + swift integration
      group = @project.main_group.new_group('SwiftMainIntegration')

      [
        'platforms/ios/iosvpnprotocol.swift',
        'platforms/ios/ioslogger.swift',
      ].each { |filename|
        file = group.new_file(filename)
        @target_main.add_file_references([file])
      }
    end

    if (platform == 'ios' && adjust_sdk_token != "")
      frameworks_group = @project.groups.find { |group| group.display_name == 'Frameworks' }
      frameworks_build_phase = @target_main.build_phases.find { |build_phase| build_phase.to_s == 'FrameworksBuildPhase' }

      framework_ref = frameworks_group.new_file('AdServices.framework')
      build_file = frameworks_build_phase.add_file_reference(framework_ref)
      build_file.settings = { 'ATTRIBUTES' => ['Weak'] }

      framework_ref = frameworks_group.new_file('iAd.framework')
      frameworks_build_phase.add_file_reference(framework_ref)

      # Adjust SDK
      group = @project.main_group.new_group('AdjustSDK')

      [
        '3rd/adjust-ios-sdk/Adjust/ADJActivityHandler.h',
        '3rd/adjust-ios-sdk/Adjust/ADJActivityKind.h',
        '3rd/adjust-ios-sdk/Adjust/ADJActivityPackage.h',
        '3rd/adjust-ios-sdk/Adjust/ADJActivityState.h',
        '3rd/adjust-ios-sdk/Adjust/ADJAdjustFactory.h',
        '3rd/adjust-ios-sdk/Adjust/ADJAdRevenue.h',
        '3rd/adjust-ios-sdk/Adjust/ADJAttribution.h',
        '3rd/adjust-ios-sdk/Adjust/ADJAttributionHandler.h',
        '3rd/adjust-ios-sdk/Adjust/ADJBackoffStrategy.h',
        '3rd/adjust-ios-sdk/Adjust/ADJAdditions/NSData+ADJAdditions.h',
        '3rd/adjust-ios-sdk/Adjust/ADJAdditions/NSNumber+ADJAdditions.h',
        '3rd/adjust-ios-sdk/Adjust/ADJAdditions/NSString+ADJAdditions.h',
        '3rd/adjust-ios-sdk/Adjust/ADJConfig.h',
        '3rd/adjust-ios-sdk/Adjust/ADJEvent.h',
        '3rd/adjust-ios-sdk/Adjust/ADJEventFailure.h',
        '3rd/adjust-ios-sdk/Adjust/ADJEventSuccess.h',
        '3rd/adjust-ios-sdk/Adjust/ADJLinkResolution.h',
        '3rd/adjust-ios-sdk/Adjust/ADJLogger.h',
        '3rd/adjust-ios-sdk/Adjust/ADJPackageBuilder.h',
        '3rd/adjust-ios-sdk/Adjust/ADJPackageHandler.h',
        '3rd/adjust-ios-sdk/Adjust/ADJPackageParams.h',
        '3rd/adjust-ios-sdk/Adjust/ADJRequestHandler.h',
        '3rd/adjust-ios-sdk/Adjust/ADJResponseData.h',
        '3rd/adjust-ios-sdk/Adjust/ADJSdkClickHandler.h',
        '3rd/adjust-ios-sdk/Adjust/ADJSessionFailure.h',
        '3rd/adjust-ios-sdk/Adjust/ADJSessionParameters.h',
        '3rd/adjust-ios-sdk/Adjust/ADJSessionSuccess.h',
        '3rd/adjust-ios-sdk/Adjust/ADJSubscription.h',
        '3rd/adjust-ios-sdk/Adjust/ADJThirdPartySharing.h',
        '3rd/adjust-ios-sdk/Adjust/ADJTimerCycle.h',
        '3rd/adjust-ios-sdk/Adjust/ADJTimerOnce.h',
        '3rd/adjust-ios-sdk/Adjust/ADJUrlStrategy.h',
        '3rd/adjust-ios-sdk/Adjust/ADJUserDefaults.h',
        '3rd/adjust-ios-sdk/Adjust/Adjust.h',
        '3rd/adjust-ios-sdk/Adjust/ADJUtil.h',
        '3rd/adjust-ios-sdk/Adjust/ADJActivityHandler.m',
        '3rd/adjust-ios-sdk/Adjust/ADJActivityKind.m',
        '3rd/adjust-ios-sdk/Adjust/ADJActivityPackage.m',
        '3rd/adjust-ios-sdk/Adjust/ADJActivityState.m',
        '3rd/adjust-ios-sdk/Adjust/ADJAdjustFactory.m',
        '3rd/adjust-ios-sdk/Adjust/ADJAdRevenue.m',
        '3rd/adjust-ios-sdk/Adjust/ADJAttribution.m',
        '3rd/adjust-ios-sdk/Adjust/ADJAttributionHandler.m',
        '3rd/adjust-ios-sdk/Adjust/ADJBackoffStrategy.m',
        '3rd/adjust-ios-sdk/Adjust/ADJAdditions/NSData+ADJAdditions.m',
        '3rd/adjust-ios-sdk/Adjust/ADJAdditions/NSNumber+ADJAdditions.m',
        '3rd/adjust-ios-sdk/Adjust/ADJAdditions/NSString+ADJAdditions.m',
        '3rd/adjust-ios-sdk/Adjust/ADJConfig.m',
        '3rd/adjust-ios-sdk/Adjust/ADJEvent.m',
        '3rd/adjust-ios-sdk/Adjust/ADJEventFailure.m',
        '3rd/adjust-ios-sdk/Adjust/ADJEventSuccess.m',
        '3rd/adjust-ios-sdk/Adjust/ADJLinkResolution.m',
        '3rd/adjust-ios-sdk/Adjust/ADJLogger.m',
        '3rd/adjust-ios-sdk/Adjust/ADJPackageBuilder.m',
        '3rd/adjust-ios-sdk/Adjust/ADJPackageHandler.m',
        '3rd/adjust-ios-sdk/Adjust/ADJPackageParams.m',
        '3rd/adjust-ios-sdk/Adjust/ADJRequestHandler.m',
        '3rd/adjust-ios-sdk/Adjust/ADJResponseData.m',
        '3rd/adjust-ios-sdk/Adjust/ADJSdkClickHandler.m',
        '3rd/adjust-ios-sdk/Adjust/ADJSessionFailure.m',
        '3rd/adjust-ios-sdk/Adjust/ADJSessionParameters.m',
        '3rd/adjust-ios-sdk/Adjust/ADJSessionSuccess.m',
        '3rd/adjust-ios-sdk/Adjust/ADJSubscription.m',
        '3rd/adjust-ios-sdk/Adjust/ADJThirdPartySharing.m',
        '3rd/adjust-ios-sdk/Adjust/ADJTimerCycle.m',
        '3rd/adjust-ios-sdk/Adjust/ADJTimerOnce.m',
        '3rd/adjust-ios-sdk/Adjust/ADJUrlStrategy.m',
        '3rd/adjust-ios-sdk/Adjust/ADJUserDefaults.m',
        '3rd/adjust-ios-sdk/Adjust/Adjust.m',
        '3rd/adjust-ios-sdk/Adjust/ADJUtil.m',
      ].each { |filename|
        file = group.new_file(filename)
        file_reference = @target_main.add_file_references([file], '-fobjc-arc')
      }
    end
  end

  def setup_target_extension(shortVersion, fullVersion, platform, configHash)
    @target_extension = @project.new_target(:app_extension, 'WireGuardNetworkExtension', platform == 'macos' ? :osx : :ios)

    @target_extension.build_configurations.each do |config|
      config.base_configuration_reference = @configFile

      config.build_settings['LD_RUNPATH_SEARCH_PATHS'] ||= '"$(inherited) @executable_path/../Frameworks"'
      config.build_settings['SWIFT_VERSION'] ||= '5.0'
      config.build_settings['CLANG_ENABLE_MODULES'] ||= 'YES'
      config.build_settings['SWIFT_OBJC_BRIDGING_HEADER'] ||= 'macos/networkextension/WireGuardNetworkExtension-Bridging-Header.h'
      config.build_settings['SWIFT_PRECOMPILE_BRIDGING_HEADER'] = 'NO'
      config.build_settings['APPLICATION_EXTENSION_API_ONLY'] = 'YES'
#     config.build_settings['STRIP_BITCODE_FROM_COPIED_FILES'] = 'NO'
      config.build_settings['FRAMEWORK_SEARCH_PATHS'] ||= [
        "$(inherited)",
        "$(PROJECT_DIR)/3rd",
        "$(PROJECT_DIR)/3rd/OpenVPNAdapter/build/Release-iphoneos",
#       "$(PROJECT_DIR)/3rd/ShadowSocks/build/Release-iphoneos/",
#       "$(PROJECT_DIR)/3rd/PacketProcessor/build/Release-iphoneos/",
#       "$(PROJECT_DIR)/3rd/outline-go-tun2socks/build/ios/"
      ]

      # Versions and names
      config.build_settings['MARKETING_VERSION'] ||= shortVersion
      config.build_settings['CURRENT_PROJECT_VERSION'] ||= fullVersion
      config.build_settings['PRODUCT_BUNDLE_IDENTIFIER'] ||= configHash['NETEXT_ID_MACOS'] if platform == 'macos'
      config.build_settings['PRODUCT_BUNDLE_IDENTIFIER'] ||= configHash['NETEXT_ID_IOS'] if platform == 'ios'
      config.build_settings['PRODUCT_NAME'] = 'WireGuardNetworkExtension'

      # other configs
      config.build_settings['INFOPLIST_FILE'] ||= 'macos/networkextension/Info.plist'
      config.build_settings['CODE_SIGN_ENTITLEMENTS'] ||= platform + '/networkextension/AmneziaVPNNetworkExtension.entitlements'
      config.build_settings['CODE_SIGN_IDENTITY'] = 'Apple Development'

      if platform == 'ios'
        config.build_settings['ENABLE_BITCODE'] ||= 'NO'
        config.build_settings['SDKROOT'] = 'iphoneos'

        config.build_settings['OTHER_LDFLAGS'] ||= [
          "-stdlib=libc++",
          "-Wl,-rpath,@executable_path/Frameworks",
          "-framework",
          "AssetsLibrary",
          "-framework",
          "MobileCoreServices",
          "-lm",
          "-framework",
          "UIKit",
          "-lz",
          "-framework",
          "OpenGLES",
        ]
      end

      groupId = "";
      if (platform == 'macos')
        groupId = configHash['DEVELOPMENT_TEAM'] + "." + configHash['GROUP_ID_MACOS']
      else
        groupId = configHash['GROUP_ID_IOS']
      end

      config.build_settings['GCC_PREPROCESSOR_DEFINITIONS'] ||= [
        # This is needed to compile the iosglue without Qt.
        'NETWORK_EXTENSION=1',
        'GROUP_ID=\"' + groupId + '\"',
      ]

      if config.name == 'Release'
        config.build_settings['SWIFT_OPTIMIZATION_LEVEL'] ||= '-Onone'
      end

    end

    group = @project.main_group.new_group('WireGuardExtension')
    [
      '3rd/wireguard-apple/Sources/WireGuardKit/WireGuardAdapter.swift',
      '3rd/wireguard-apple/Sources/WireGuardKit/PacketTunnelSettingsGenerator.swift',
      '3rd/wireguard-apple/Sources/WireGuardKit/DNSResolver.swift',
      '3rd/wireguard-apple/Sources/WireGuardNetworkExtension/ErrorNotifier.swift',
      '3rd/wireguard-apple/Sources/Shared/Keychain.swift',
      '3rd/wireguard-apple/Sources/Shared/Model/TunnelConfiguration+WgQuickConfig.swift',
      '3rd/wireguard-apple/Sources/Shared/Model/NETunnelProviderProtocol+Extension.swift',
      '3rd/wireguard-apple/Sources/Shared/Model/String+ArrayConversion.swift',
      '3rd/wireguard-apple/Sources/WireGuardKit/TunnelConfiguration.swift',
      '3rd/wireguard-apple/Sources/WireGuardKit/IPAddressRange.swift',
      '3rd/wireguard-apple/Sources/WireGuardKit/Endpoint.swift',
      '3rd/wireguard-apple/Sources/WireGuardKit/DNSServer.swift',
      '3rd/wireguard-apple/Sources/WireGuardKit/InterfaceConfiguration.swift',
      '3rd/wireguard-apple/Sources/WireGuardKit/PeerConfiguration.swift',
      '3rd/wireguard-apple/Sources/Shared/FileManager+Extension.swift',
      '3rd/wireguard-apple/Sources/WireGuardKitC/x25519.c',
      '3rd/wireguard-apple/Sources/WireGuardKit/Array+ConcurrentMap.swift',
      '3rd/wireguard-apple/Sources/WireGuardKit/IPAddress+AddrInfo.swift',
      '3rd/wireguard-apple/Sources/WireGuardKit/PrivateKey.swift',
    ].each { |filename|
      file = group.new_file(filename)
      @target_extension.add_file_references([file])
    }
    # @target_extension + swift integration
    group = @project.main_group.new_group('SwiftExtensionIntegration')

    [
      'platforms/ios/iostunnel.swift',
      'platforms/ios/iosglue.mm',
      'platforms/ios/ioslogger.swift',
#     'platforms/ios/Shadowsocks.h',
#     'platforms/ios/Shadowsocks.m',
#     'platforms/ios/ShadowsocksConnectivity.h',
#     'platforms/ios/ShadowsocksConnectivity.m',
#     'platforms/ios/Subnet.swift',
    ].each { |filename|
      file = group.new_file(filename)
      @target_extension.add_file_references([file])
    }

    frameworks_group = @project.groups.find { |group| group.display_name == 'Frameworks' }
    frameworks_build_phase = @target_extension.build_phases.find { |build_phase| build_phase.to_s == 'FrameworksBuildPhase' }

    frameworks_build_phase.clear

    framework_ref = frameworks_group.new_file('libwg-go.a')
    frameworks_build_phase.add_file_reference(framework_ref)

    framework_ref = frameworks_group.new_file('NetworkExtension.framework')
    frameworks_build_phase.add_file_reference(framework_ref)
    
    framework_ref = frameworks_group.new_file('3rd/OpenVPNAdapter/build/Release-iphoneos/LZ4.framework')
    frameworks_build_phase.add_file_reference(framework_ref)
    
    framework_ref = frameworks_group.new_file('3rd/OpenVPNAdapter/build/Release-iphoneos/mbedTLS.framework')
    frameworks_build_phase.add_file_reference(framework_ref)
    
    framework_ref = frameworks_group.new_file('3rd/OpenVPNAdapter/build/Release-iphoneos/OpenVPNClient.framework')
    frameworks_build_phase.add_file_reference(framework_ref)
    
    framework_ref = frameworks_group.new_file('3rd/OpenVPNAdapter/build/Release-iphoneos/OpenVPNAdapter.framework')
    frameworks_build_phase.add_file_reference(framework_ref)
    
#   framework_ref = frameworks_group.new_file('3rd/ShadowSocks/build/Release-iphoneos/ShadowSocks.framework')
#   frameworks_build_phase.add_file_reference(framework_ref)
#   
#   framework_ref = frameworks_group.new_file('3rd/PacketProcessor/PacketProcessor/CocoaAsyncSocket/CocoaAsyncSocket.framework')
#   frameworks_build_phase.add_file_reference(framework_ref)
#   
#   framework_ref = frameworks_group.new_file('3rd/outline-go-tun2socks/build/ios/Tun2socks.framework')
#   frameworks_build_phase.add_file_reference(framework_ref)
    

    # This fails: @target_main.add_dependency @target_extension
    container_proxy = @project.new(Xcodeproj::Project::PBXContainerItemProxy)
    container_proxy.container_portal = @project.root_object.uuid
    container_proxy.proxy_type = Xcodeproj::Constants::PROXY_TYPES[:native_target]
    container_proxy.remote_global_id_string = @target_extension.uuid
    container_proxy.remote_info = @target_extension.name

    dependency = @project.new(Xcodeproj::Project::PBXTargetDependency)
    dependency.name = @target_extension.name
    dependency.target = @target_main
    dependency.target_proxy = container_proxy

    @target_main.dependencies << dependency

    copy_appex = @target_main.new_copy_files_build_phase
    copy_appex.name = 'Copy Network-Extension plugin'
    copy_appex.symbol_dst_subfolder_spec = :plug_ins

    appex_file = copy_appex.add_file_reference @target_extension.product_reference
    appex_file.settings = { "ATTRIBUTES" => ['RemoveHeadersOnCopy'] }
  end

  def setup_target_gobridge
    target_gobridge = legacy_target = @project.new(Xcodeproj::Project::PBXLegacyTarget)

    target_gobridge.build_working_directory = 'macos/gobridge'
    target_gobridge.build_tool_path = 'make'
    target_gobridge.pass_build_settings_in_environment = '1'
    target_gobridge.build_arguments_string = '$(ACTION)'
    target_gobridge.name = 'WireGuardGoBridge'
    target_gobridge.product_name = 'WireGuardGoBridge'

    @project.targets << target_gobridge
    @target_extension.add_dependency target_gobridge
  end

  def setup_target_balrog
    target_balrog = legacy_target = @project.new(Xcodeproj::Project::PBXLegacyTarget)

    target_balrog.build_working_directory = 'balrog'
    target_balrog.build_tool_path = 'make'
    target_balrog.pass_build_settings_in_environment = '1'
    target_balrog.build_arguments_string = '$(ACTION)'
    target_balrog.name = 'WireGuardBalrog'
    target_balrog.product_name = 'WireGuardBalrog'

    @project.targets << target_balrog

    frameworks_group = @project.groups.find { |group| group.display_name == 'Frameworks' }
    frameworks_build_phase = @target_main.build_phases.find { |build_phase| build_phase.to_s == 'FrameworksBuildPhase' }

    framework_ref = frameworks_group.new_file('balrog/balrog.a')
    frameworks_build_phase.add_file_reference(framework_ref)

    # This fails: @target_main.add_dependency target_balrog
    container_proxy = @project.new(Xcodeproj::Project::PBXContainerItemProxy)
    container_proxy.container_portal = @project.root_object.uuid
    container_proxy.proxy_type = Xcodeproj::Constants::PROXY_TYPES[:native_target]
    container_proxy.remote_global_id_string = target_balrog.uuid
    container_proxy.remote_info = target_balrog.name

    dependency = @project.new(Xcodeproj::Project::PBXTargetDependency)
    dependency.name = target_balrog.name
    dependency.target = @target_main
    dependency.target_proxy = container_proxy

    @target_main.dependencies << dependency
  end

  def setup_target_wireguardtools
    target_wireguardtools = legacy_target = @project.new(Xcodeproj::Project::PBXLegacyTarget)

    target_wireguardtools.build_working_directory = '3rd/wireguard-tools/src'
    target_wireguardtools.build_tool_path = 'make'
    target_wireguardtools.pass_build_settings_in_environment = '1'
    target_wireguardtools.build_arguments_string = '$(ACTION)'
    target_wireguardtools.name = 'WireGuardTools'
    target_wireguardtools.product_name = 'WireGuardTools'

    @project.targets << target_wireguardtools

    # This fails: @target_main.add_dependency target_wireguardtools
    container_proxy = @project.new(Xcodeproj::Project::PBXContainerItemProxy)
    container_proxy.container_portal = @project.root_object.uuid
    container_proxy.proxy_type = Xcodeproj::Constants::PROXY_TYPES[:native_target]
    container_proxy.remote_global_id_string = target_wireguardtools.uuid
    container_proxy.remote_info = target_wireguardtools.name

    dependency = @project.new(Xcodeproj::Project::PBXTargetDependency)
    dependency.name = target_wireguardtools.name
    dependency.target = @target_main
    dependency.target_proxy = container_proxy

    @target_main.dependencies << dependency

    copy_wireguardTools = @target_main.new_copy_files_build_phase
    copy_wireguardTools.name = 'Copy wireguard-tools'
    copy_wireguardTools.symbol_dst_subfolder_spec = :wrapper
    copy_wireguardTools.dst_path = 'Contents/Resources/utils'

    group = @project.main_group.new_group('WireGuardTools')
    file = group.new_file '3rd/wireguard-tools/src/wg'

    wireguardTools_file = copy_wireguardTools.add_file_reference file
    wireguardTools_file.settings = { "ATTRIBUTES" => ['RemoveHeadersOnCopy'] }
  end

  def setup_target_wireguardgo
    target_wireguardgo = legacy_target = @project.new(Xcodeproj::Project::PBXLegacyTarget)

    target_wireguardgo.build_working_directory = '3rd/wireguard-go'
    target_wireguardgo.build_tool_path = 'make'
    target_wireguardgo.pass_build_settings_in_environment = '1'
    target_wireguardgo.build_arguments_string = '$(ACTION)'
    target_wireguardgo.name = 'WireGuardGo'
    target_wireguardgo.product_name = 'WireGuardGo'

    @project.targets << target_wireguardgo

    # This fails: @target_main.add_dependency target_wireguardgo
    container_proxy = @project.new(Xcodeproj::Project::PBXContainerItemProxy)
    container_proxy.container_portal = @project.root_object.uuid
    container_proxy.proxy_type = Xcodeproj::Constants::PROXY_TYPES[:native_target]
    container_proxy.remote_global_id_string = target_wireguardgo.uuid
    container_proxy.remote_info = target_wireguardgo.name

    dependency = @project.new(Xcodeproj::Project::PBXTargetDependency)
    dependency.name = target_wireguardgo.name
    dependency.target = @target_main
    dependency.target_proxy = container_proxy

    @target_main.dependencies << dependency

    copy_wireguardGo = @target_main.new_copy_files_build_phase
    copy_wireguardGo.name = 'Copy wireguard-go'
    copy_wireguardGo.symbol_dst_subfolder_spec = :wrapper
    copy_wireguardGo.dst_path = 'Contents/Resources/utils'

    group = @project.main_group.new_group('WireGuardGo')
    file = group.new_file '3rd/wireguard-go/wireguard-go'

    wireguardGo_file = copy_wireguardGo.add_file_reference file
    wireguardGo_file.settings = { "ATTRIBUTES" => ['RemoveHeadersOnCopy'] }
  end

  def setup_target_loginitem(shortVersion, fullVersion, configHash)
    @target_loginitem = @project.new_target(:application, 'AmneziaVPNLoginItem', :osx)

    @target_loginitem.build_configurations.each do |config|
      config.base_configuration_reference = @configFile

      config.build_settings['LD_RUNPATH_SEARCH_PATHS'] ||= '"$(inherited) @executable_path/../Frameworks"'

      # Versions and names
      config.build_settings['MARKETING_VERSION'] ||= shortVersion
      config.build_settings['CURRENT_PROJECT_VERSION'] ||= fullVersion
      config.build_settings['PRODUCT_BUNDLE_IDENTIFIER'] ||= configHash['LOGIN_ID_MACOS']
      config.build_settings['PRODUCT_NAME'] = 'AmneziaVPNLoginItem'

      # other configs
      config.build_settings['INFOPLIST_FILE'] ||= 'macos/loginitem/Info.plist'
      config.build_settings['CODE_SIGN_ENTITLEMENTS'] ||= 'macos/loginitem/MozillaVPNLoginItem.entitlements'
      config.build_settings['CODE_SIGN_IDENTITY'] = 'Apple Development'
      config.build_settings['SKIP_INSTALL'] = 'YES'

      config.build_settings['GCC_PREPROCESSOR_DEFINITIONS'] ||= [
        'APP_ID=\"' + configHash['APP_ID_MACOS'] + '\"',
      ]

      if config.name == 'Release'
        config.build_settings['SWIFT_OPTIMIZATION_LEVEL'] ||= '-Onone'
      end
    end

    group = @project.main_group.new_group('LoginItem')
    [
      'macos/loginitem/main.m',
    ].each { |filename|
      file = group.new_file(filename)
      @target_loginitem.add_file_references([file])
    }

    # This fails: @target_main.add_dependency @target_loginitem
    container_proxy = @project.new(Xcodeproj::Project::PBXContainerItemProxy)
    container_proxy.container_portal = @project.root_object.uuid
    container_proxy.proxy_type = Xcodeproj::Constants::PROXY_TYPES[:native_target]
    container_proxy.remote_global_id_string = @target_loginitem.uuid
    container_proxy.remote_info = @target_loginitem.name

    dependency = @project.new(Xcodeproj::Project::PBXTargetDependency)
    dependency.name = @target_loginitem.name
    dependency.target = @target_main
    dependency.target_proxy = container_proxy

    @target_main.dependencies << dependency

    copy_app = @target_main.new_copy_files_build_phase
    copy_app.name = 'Copy LoginItem'
    copy_app.symbol_dst_subfolder_spec = :wrapper
    copy_app.dst_path = 'Contents/Library/LoginItems'

    app_file = copy_app.add_file_reference @target_loginitem.product_reference
    app_file.settings = { "ATTRIBUTES" => ['RemoveHeadersOnCopy'] }
  end

  def setup_target_nativemessaging(shortVersion, fullVersion, configHash)
    @target_nativemessaging = @project.new_target(:application, 'AmneziaVPNNativeMessaging', :osx)

    @target_nativemessaging.build_configurations.each do |config|
      config.base_configuration_reference = @configFile

      config.build_settings['LD_RUNPATH_SEARCH_PATHS'] ||= '"$(inherited) @executable_path/../Frameworks"'

      # Versions and names
      config.build_settings['MARKETING_VERSION'] ||= shortVersion
      config.build_settings['CURRENT_PROJECT_VERSION'] ||= fullVersion
      config.build_settings['PRODUCT_BUNDLE_IDENTIFIER'] ||= configHash['NATIVEMESSAGING_ID_MACOS']
      config.build_settings['PRODUCT_NAME'] = 'AmneziaVPNNativeMessaging'

      # other configs
      config.build_settings['INFOPLIST_FILE'] ||= 'macos/nativeMessaging/Info.plist'
      config.build_settings['CODE_SIGN_ENTITLEMENTS'] ||= 'macos/nativeMessaging/MozillaVPNNativeMessaging.entitlements'
      config.build_settings['CODE_SIGN_IDENTITY'] = 'Apple Development'
      config.build_settings['SKIP_INSTALL'] = 'YES'
    end

    group = @project.main_group.new_group('NativeMessaging')
    [
      'extension/app/constants.h',
      'extension/app/handler.cpp',
      'extension/app/handler.h',
      'extension/app/json.hpp',
      'extension/app/logger.cpp',
      'extension/app/logger.h',
      'extension/app/main.cpp',
      'extension/app/vpnconnection.cpp',
      'extension/app/vpnconnection.h',
    ].each { |filename|
      file = group.new_file(filename)
      @target_nativemessaging.add_file_references([file])
    }

    # This fails: @target_main.add_dependency @target_nativemessaging
    container_proxy = @project.new(Xcodeproj::Project::PBXContainerItemProxy)
    container_proxy.container_portal = @project.root_object.uuid
    container_proxy.proxy_type = Xcodeproj::Constants::PROXY_TYPES[:native_target]
    container_proxy.remote_global_id_string = @target_nativemessaging.uuid
    container_proxy.remote_info = @target_nativemessaging.name

    dependency = @project.new(Xcodeproj::Project::PBXTargetDependency)
    dependency.name = @target_nativemessaging.name
    dependency.target = @target_main
    dependency.target_proxy = container_proxy

    @target_main.dependencies << dependency

    copy_app = @target_main.new_copy_files_build_phase
    copy_app.name = 'Copy LoginItem'
    copy_app.symbol_dst_subfolder_spec = :wrapper
    copy_app.dst_path = 'Contents/Library/NativeMessaging'

    app_file = copy_app.add_file_reference @target_nativemessaging.product_reference
    app_file.settings = { "ATTRIBUTES" => ['RemoveHeadersOnCopy'] }

    copy_nativeMessagingManifest = @target_main.new_copy_files_build_phase
    copy_nativeMessagingManifest.name = 'Copy native messaging manifest'
    copy_nativeMessagingManifest.symbol_dst_subfolder_spec = :wrapper
    copy_nativeMessagingManifest.dst_path = 'Contents/Resources/utils'

    group = @project.main_group.new_group('WireGuardHelper')
    file = group.new_file 'extension/app/manifests/macos/mozillavpn.json'

    nativeMessagingManifest_file = copy_nativeMessagingManifest.add_file_reference file
    nativeMessagingManifest_file.settings = { "ATTRIBUTES" => ['RemoveHeadersOnCopy'] }
  end

  def die(msg)
   print $msg
   exit 1
  end
end

if ARGV.length < 4 || (ARGV[3] != "ios" && ARGV[3] != "macos")
  puts "Usage: <script> project_file shortVersion fullVersion ios/macos"
  exit 1
end

if !File.exist?("xcode.xconfig")
  puts "xcode.xconfig file is required! See the template file."
  exit 1
end

config = Hash.new
configFile = File.read("xcode.xconfig").split("\n")
configFile.each { |line|
  next if line[0] == "#"

  if line.include? "="
    keys = line.split("=")
    config[keys[0].strip] = keys[1].strip
  end
}

platform = "macos"
platform = "ios" if ARGV[3] == "ios"
networkExtension = true if ARGV[4] == "1"
adjust_sdk_token = ARGV[5]

r = XCodeprojPatcher.new
r.run ARGV[0], ARGV[1], ARGV[2], platform, networkExtension, config, adjust_sdk_token
exit 0
