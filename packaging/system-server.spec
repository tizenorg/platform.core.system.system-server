%bcond_with x

#sbs-git:slp/pkgs/s/system-server system-server 0.1.51 56e16bca39f96d6c8aed9ed3df2fea9b393801be
Name:       system-server
Summary:    System server
Version:    2.0.0
Release:    0
Group:      System/Service
License:    Apache-2.0
Source0:    system-server-%{version}.tar.gz
Source1:    system-server.manifest
Source2:    deviced.manifest
Source3:    sysman.manifest
Source4:    libslp-pm.manifest
Source5:    haptic.manifest
Source6:    devman.manifest
Source8:    regpmon.service
Source9:    zbooting-done.service
BuildRequires:  cmake
BuildRequires:  libattr-devel
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(heynoti)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(tapi)
BuildRequires:  pkgconfig(edbus)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(syspopup-caller)
%if %{with x}
BuildRequires:  pkgconfig(x11)
%endif
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(usbutils)
BuildRequires:  pkgconfig(udev)
BuildRequires:  pkgconfig(device-node)
BuildRequires:  pkgconfig(libsmack)
BuildRequires:  gettext
BuildRequires:  pkgconfig(sensor)
BuildRequires:  pkgconfig(libsystemd-daemon)
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(libtzplatform-config)

%{?systemd_requires}
Requires(post): /usr/bin/vconftool

%description
system server

%package -n sysman
Summary:    Sysman library
Group:      System/Libraries
Requires:   %{name} = %{version}-%{release}

%description -n sysman
System manager interface library.

%package -n sysman-devel
Summary:    Sysman devel library
Group:      System/Development
Requires:   sysman = %{version}-%{release}

%description -n sysman-devel
sysman devel library.

%package -n sysman-internal-devel
Summary:    Sysman internal devel library
Group:      System/Development
Requires:   sysman = %{version}-%{release}

%description -n sysman-internal-devel
sysman internal devel library.

%package -n libslp-pm
Summary:    Power manager client
Group:      System/Libraries
Requires:   %{name} = %{version}-%{release}

%description -n libslp-pm
power-manager library.

%package -n libslp-pm-devel
Summary:    Power manager client (devel)
Group:      System/Development
Requires:   libslp-pm = %{version}-%{release}

%description -n libslp-pm-devel
power-manager devel library.

%package -n libhaptic
Summary:    Haptic library
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description -n libhaptic
Haptic library for device control

%package -n libhaptic-devel
Summary:    Haptic library for (devel)
Group:      Development/Libraries
Requires:   libhaptic = %{version}-%{release}

%description -n libhaptic-devel
Haptic library for device control (devel)

%package -n libhaptic-plugin-devel
Summary:    Haptic plugin library for (devel)
Group:      Development/Libraries
Requires:   libhaptic = %{version}-%{release}

%description -n libhaptic-plugin-devel
Haptic plugin library for device control (devel)

%package -n libdevman
Summary:    Device manager library
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description -n libdevman
Device manager library for device control

%package -n libdevman-devel
Summary:    Device manager library for (devel)
Group:      Development/Libraries
Requires:   libdevman = %{version}-%{release}

%description -n libdevman-devel
Device manager library for device control (devel)

%package -n libdevman-haptic-devel
Summary:    Haptic Device manager library for (devel)
Group:      Development/Libraries
Requires:   libdevman-devel = %{version}-%{release}

%description -n libdevman-haptic-devel
Haptic Device manager library for device control (devel)

%package -n libdeviced
Summary:    Deviced library
Group:      System/Libraries
Requires:   %{name} = %{version}-%{release}

%description -n libdeviced
Deviced library for device control

%package -n libdeviced-devel
Summary:    Deviced library for (devel)
Group:      System/Development
Requires:   libdeviced = %{version}-%{release}

%description -n libdeviced-devel
Deviced library for device control (devel)

%prep
%setup -q

%cmake . \
    -DCMAKE_INSTALL_PREFIX=%{_prefix} \
%if %{with x}
    -DX11_SUPPORT=On \
%else
    -DX11_SUPPORT=Off \
%endif
    #eol

%build
cp %{SOURCE1} .
cp %{SOURCE2} .
cp %{SOURCE3} .
cp %{SOURCE4} .
cp %{SOURCE5} .
cp %{SOURCE6} .
%cmake . -DTZ_SYS_ETC=%TZ_SYS_ETC

%install
%make_install

%install_service multi-user.target.wants system-server.service
%install_service sockets.target.wants system-server.socket

%install_service graphical.target.wants regpmon.service
install -m 0644 %{SOURCE8} %{buildroot}%{_unitdir}/regpmon.service

%install_service graphical.target.wants zbooting-done.service
install -m 0644 %{SOURCE9} %{buildroot}%{_unitdir}/zbooting-done.service

%if 0%{?simulator}
rm -f %{buildroot}%{_bindir}/restart
%endif

%post
#memory type vconf key init
users_gid=$(getent group $TZ_SYS_USER_GROUP | cut -f3 -d':')

vconftool set -t int memory/sysman/usbhost_status -1 -i
vconftool set -t int memory/sysman/mmc 0 -i
vconftool set -t int memory/sysman/earjack_key 0 -i
vconftool set -t int memory/sysman/added_usb_storage 0 -i
vconftool set -t int memory/sysman/removed_usb_storage 0 -i
vconftool set -t int memory/sysman/charger_status -1 -i
vconftool set -t int memory/sysman/charge_now -1 -i
vconftool set -t int memory/sysman/battery_status_low -1 -i
vconftool set -t int memory/sysman/battery_capacity -1 -i
vconftool set -t int memory/sysman/usb_status -1 -i
vconftool set -t int memory/sysman/earjack -1 -i
vconftool set -t int memory/sysman/low_memory 1 -i
vconftool set -t int memory/sysman/sliding_keyboard -1 -i
vconftool set -t int memory/sysman/mmc_mount -1 -i
vconftool set -t int memory/sysman/mmc_unmount -1 -i
vconftool set -t int memory/sysman/mmc_format -1 -i
vconftool set -t int memory/sysman/mmc_format_progress 0 -i
vconftool set -t int memory/sysman/mmc_err_status 0 -i
vconftool set -t int memory/sysman/power_off 0 -g $users_gid -i -f
vconftool set -t int memory/sysman/battery_level_status -1 -i
vconftool set -t string memory/private/sysman/added_storage_uevent "" -i
vconftool set -t string memory/private/sysman/removed_storage_uevent "" -g $users_gid -i

vconftool set -t int memory/sysman/hdmi 0 -i

vconftool set -t int memory/sysman/stime_changed 0 -i

#db type vconf key init
vconftool set -t int db/sysman/mmc_dev_changed 0 -i

vconftool set -t int memory/pm/state 0 -i -g $users_gid
vconftool set -t int memory/pm/battery_timetofull -1 -i
vconftool set -t int memory/pm/battery_timetoempty -1 -i
vconftool set -t int memory/pm/sip_status 0 -i -g $users_gid
vconftool set -t int memory/pm/custom_brightness_status 0 -i -g $users_gid
vconftool set -t bool memory/pm/brt_changed_lpm 0 -i
vconftool set -t int memory/pm/current_brt 60 -i -g $users_gid

heynotitool set system_wakeup
heynotitool set pm_event

heynotitool set power_off_start

heynotitool set mmcblk_add
heynotitool set mmcblk_remove
heynotitool set device_charge_chgdet
heynotitool set device_usb_host_add
heynotitool set device_usb_host_remove
heynotitool set device_pci_keyboard_add
heynotitool set device_pci_keyboard_remove

heynotitool set device_usb_chgdet
heynotitool set device_ta_chgdet
heynotitool set device_earjack_chgdet
heynotitool set device_earkey_chgdet
heynotitool set device_tvout_chgdet
heynotitool set device_hdmi_chgdet
heynotitool set device_keyboard_chgdet


systemctl daemon-reload
if [ "$1" = "1" ]; then
    systemctl restart system-server.service
    systemctl restart regpmon.service
	systemctl restart zbooting-done.service
fi
/sbin/ldconfig

%preun
if [ "$1" = "0" ]; then
    systemctl stop system-server.service
    systemctl stop regpmon.service
	systemctl stop zbooting-done.service
fi

%postun
systemctl daemon-reload
/sbin/ldconfig

%post -n sysman -p /sbin/ldconfig

%postun -n sysman -p /sbin/ldconfig

%post -n libslp-pm -p /sbin/ldconfig

%postun -n libslp-pm -p /sbin/ldconfig

%post -n libhaptic -p /sbin/ldconfig

%postun -n libhaptic -p /sbin/ldconfig

%post -n libdevman -p /sbin/ldconfig

%postun -n libdevman -p /sbin/ldconfig

%post -n libdeviced -p /sbin/ldconfig

%postun -n libdeviced -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%license LICENSE.APLv2
%config %{_sysconfdir}/dbus-1/system.d/deviced.conf
%{_bindir}/system_server
%{_libdir}/system-server/shutdown.sh
%if %{undefined simulator}
%{_bindir}/restart
%endif
%{_bindir}/movi_format.sh
%{_bindir}/sys_event
%{_bindir}/pm_event
%{_bindir}/regpmon
%{_bindir}/set_pmon
%{_bindir}/sys_pci_noti
%{_bindir}/mmc-smack-label
%{_bindir}/device-daemon
%{_bindir}/fsck_msdosfs
%{_unitdir}/multi-user.target.wants/system-server.service
%{_unitdir}/graphical.target.wants/regpmon.service
%{_unitdir}/sockets.target.wants/system-server.socket
%{_unitdir}/system-server.service
%{_unitdir}/system-server.socket
%{_unitdir}/regpmon.service
%{_unitdir}/graphical.target.wants/zbooting-done.service
%{_unitdir}/zbooting-done.service
%{_datadir}/system-server/sys_pci_noti/res/locale/*/LC_MESSAGES/*.mo
%{_datadir}/license/fsck_msdosfs

%files -n sysman
%manifest sysman.manifest
%defattr(-,root,root,-)
%{_libdir}/libsysman.so.*

%files -n sysman-devel
%defattr(-,root,root,-)
%{_includedir}/sysman/sysman.h
%{_includedir}/sysman/sysman_managed.h
%{_includedir}/sysman/sysman_PG.h
%{_libdir}/pkgconfig/sysman.pc
%{_libdir}/libsysman.so

%files -n sysman-internal-devel
%defattr(-,root,root,-)
%{_includedir}/sysman/sysman-internal.h

%files -n libslp-pm
%defattr(-,root,root,-)
%manifest libslp-pm.manifest
%{_libdir}/libpmapi.so.*

%files -n libslp-pm-devel
%defattr(-,root,root,-)
%{_includedir}/pmapi/pmapi.h
%{_includedir}/pmapi/pmapi_managed.h
%{_includedir}/pmapi/pm_PG.h
%{_libdir}/pkgconfig/pmapi.pc
%{_libdir}/libpmapi.so

%files -n libhaptic
%defattr(-,root,root,-)
%{_libdir}/libhaptic.so.*
%manifest haptic.manifest

%files -n libhaptic-devel
%defattr(-,root,root,-)
%{_includedir}/haptic/haptic.h
%{_libdir}/libhaptic.so
%{_libdir}/pkgconfig/haptic.pc

%files -n libhaptic-plugin-devel
%defattr(-,root,root,-)
%{_includedir}/haptic/haptic_module.h
%{_includedir}/haptic/haptic_plugin_intf.h
%{_includedir}/haptic/haptic_PG.h
%{_libdir}/pkgconfig/haptic-plugin.pc

%files -n libdevman
%{_bindir}/display_wd
%{_libdir}/libdevman.so.*
%manifest devman.manifest

%files -n libdevman-devel
%{_includedir}/devman/devman.h
%{_includedir}/devman/devman_image.h
%{_includedir}/devman/devman_managed.h
%{_includedir}/devman/devman_haptic.h
%{_includedir}/devman/devman_PG.h
%{_libdir}/pkgconfig/devman.pc
%{_libdir}/libdevman.so

%files -n libdevman-haptic-devel
%{_includedir}/devman/devman_haptic_ext.h
%{_includedir}/devman/devman_haptic_ext_core.h
%{_libdir}/pkgconfig/devman_haptic.pc

%files -n libdeviced
%defattr(-,root,root,-)
%{_libdir}/libdeviced.so.*
%manifest deviced.manifest

%files -n libdeviced-devel
%defattr(-,root,root,-)
%{_includedir}/deviced/dd-battery.h
%{_includedir}/deviced/dd-control.h
%{_includedir}/deviced/dd-deviced.h
%{_includedir}/deviced/dd-deviced-managed.h
%{_includedir}/deviced/dd-display.h
%{_includedir}/deviced/dd-haptic.h
%{_includedir}/deviced/dd-led.h
%{_includedir}/deviced/haptic-module.h
%{_includedir}/deviced/haptic-plugin-intf.h
%{_libdir}/libdeviced.so
%{_libdir}/pkgconfig/deviced.pc

