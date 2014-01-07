#sbs-git:slp/pkgs/s/system-server system-server 0.1.51 56e16bca39f96d6c8aed9ed3df2fea9b393801be
Name:       system-server
Summary:    System server
Version:    0.1.65
Release:    7
Group:      System/Service
License:    Apache-2.0
Source0:    system-server-%{version}.tar.gz
Source1:    system-server.manifest
Source2:    deviced.manifest
Source3:    sysman.manifest
Source4:    libslp-pm.manifest
Source5:    haptic.manifest
Source6:    devman.manifest
BuildRequires:  cmake
BuildRequires:  libattr-devel
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(heynoti)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(tapi)
BuildRequires:  pkgconfig(edbus)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(syspopup-caller)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(usbutils)
BuildRequires:  pkgconfig(udev)
BuildRequires:  pkgconfig(device-node)
BuildRequires:  pkgconfig(libsmack)
BuildRequires:  gettext
BuildRequires:  pkgconfig(sensor)
BuildRequires:  pkgconfig(libsystemd-daemon)
BuildRequires:  pkgconfig(capi-base-common)
%{?systemd_requires}
Requires(preun): /usr/bin/systemctl
Requires(post): /usr/bin/systemctl
Requires(post): /usr/bin/vconftool
Requires(postun): /usr/bin/systemctl

%description
system server

%package system-server
Summary:    System-server daemon
Group:      System/Service
Requires:   %{name} = %{version}-%{release}

%description system-server
system server daemon.

%package -n sysman
Summary:    Sysman library
License:    Apache-2.0
Group:      System/Libraries
Requires:   %{name} = %{version}-%{release}

%description -n sysman
sysman library.

%package -n sysman-devel
Summary:    Sysman devel library
License:    Apache-2.0
Group:      System/Development
Requires:   %{name} = %{version}-%{release}

%description -n sysman-devel
sysman devel library.

%package -n sysman-internal-devel
Summary:    Sysman internal devel library
License:    Apache-2.0
Group:      System/Development
Requires:   %{name} = %{version}-%{release}

%description -n sysman-internal-devel
sysman internal devel library.

%package -n libslp-pm
Summary:    power manager client
Group:      System/Libraries
Requires:   %{name} = %{version}-%{release}

%description -n libslp-pm
power-manager library.

%package -n libslp-pm-devel
Summary:    power manager client (devel)
Group:      System/Development
Requires:   %{name} = %{version}-%{release}
#Requires:   libslp-pm

%description -n libslp-pm-devel
power-manager devel library.

%package -n libhaptic
Summary:    Haptic library
Group:      Development/Libraries

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

%description -n libhaptic-plugin-devel
Haptic plugin library for device control (devel)

%package -n libdevman
Summary:    Device manager library
Group:      Development/Libraries

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
%cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}

%build
cp %{SOURCE1} .
cp %{SOURCE2} .
cp %{SOURCE3} .
cp %{SOURCE4} .
cp %{SOURCE5} .
cp %{SOURCE6} .
%cmake .

%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}%{_unitdir}/multi-user.target.wants
mkdir -p %{buildroot}%{_unitdir}/sockets.target.wants
ln -s ../system-server.service %{buildroot}%{_unitdir}/multi-user.target.wants/system-server.service
ln -s ../system-server.service %{buildroot}%{_unitdir}/sockets.target.wants/system-server.socket

%post
#memory type vconf key init
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
vconftool set -t int memory/sysman/power_off 0 -u 5000 -i -f
vconftool set -t int memory/sysman/battery_level_status -1 -i
vconftool set -t string memory/private/sysman/added_storage_uevent "" -i
vconftool set -t string memory/private/sysman/removed_storage_uevent "" -u 5000 -i

vconftool set -t int memory/sysman/hdmi 0 -i

vconftool set -t int memory/sysman/stime_changed 0 -i

#db type vconf key init
vconftool set -t int db/sysman/mmc_dev_changed 0 -i

vconftool set -t int memory/pm/state 0 -i -g 5000
vconftool set -t int memory/pm/battery_timetofull -1 -i
vconftool set -t int memory/pm/battery_timetoempty -1 -i
vconftool set -t int memory/pm/sip_status 0 -i -g 5000
vconftool set -t int memory/pm/custom_brightness_status 0 -i -g 5000
vconftool set -t bool memory/pm/brt_changed_lpm 0 -i
vconftool set -t int memory/pm/current_brt 60 -i -g 5000

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


mkdir -p /etc/udev/rules.d
if ! [ -L /etc/udev/rules.d/91-system-server.rules ]; then
        ln -s %{_datadir}/system-server/udev-rules/91-system-server.rules /etc/udev/rules.d/91-system-server.rules
fi

systemctl daemon-reload
if [ $1 == 1 ]; then
    systemctl restart system-server.service
fi
/sbin/ldconfig

%preun
if [ $1 == 0 ]; then
    systemctl stop system-server.service
fi

%postun
systemctl daemon-reload
/sbin/ldconfig

%files -n system-server
%manifest %{name}.manifest
%license LICENSE.APLv2
%config %{_sysconfdir}/dbus-1/system.d/system-server.conf
%{_bindir}/system_server
/opt/etc/smack/accesses.d/system-server.rule
%{_libdir}/system-server/shutdown.sh
%if 0%{?simulator}
%exclude %{_bindir}/restart
%else
%{_bindir}/restart
%endif
%{_bindir}/movi_format.sh
%{_bindir}/sys_event
%{_bindir}/pm_event
%{_bindir}/sys_pci_noti
%{_bindir}/mmc-smack-label
%{_bindir}/device-daemon
%{_unitdir}/multi-user.target.wants/system-server.service
%{_unitdir}/sockets.target.wants/system-server.socket
%{_unitdir}/system-server.service
%{_unitdir}/system-server.socket
%{_datadir}/system-server/udev-rules/91-system-server.rules
%{_datadir}/system-server/sys_pci_noti/res/locale/*/LC_MESSAGES/*.mo
%config %{_sysconfdir}/dbus-1/system.d/system-server.conf

%files -n sysman
%manifest sysman.manifest
%defattr(-,root,root,-)
%{_libdir}/libsysman.so.*
%{_bindir}/regpmon
%{_bindir}/set_pmon

%post -n sysman
/sbin/ldconfig

%postun -n sysman
/sbin/ldconfig

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

%post -n libslp-pm
/sbin/ldconfig

%postun -n libslp-pm
/sbin/ldconfig

%files -n libslp-pm-devel
%defattr(-,root,root,-)
%{_includedir}/pmapi/pmapi.h
%{_includedir}/pmapi/pmapi_managed.h
%{_includedir}/pmapi/pm_PG.h
%{_libdir}/pkgconfig/pmapi.pc
%{_libdir}/libpmapi.so

%post -n libslp-pm-devel
/sbin/ldconfig

%postun -n libslp-pm-devel
/sbin/ldconfig

%files -n libhaptic
%defattr(-,root,root,-)
%{_libdir}/libhaptic.so.*
%manifest haptic.manifest

%post -n libhaptic
/sbin/ldconfig

%postun -n libhaptic
/sbin/ldconfig


%files -n libhaptic-devel
%defattr(-,root,root,-)
%{_includedir}/haptic/haptic.h
%{_libdir}/libhaptic.so
%{_libdir}/pkgconfig/haptic.pc

%post -n libhaptic-devel
/sbin/ldconfig

%postun -n libhaptic-devel
/sbin/ldconfig

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


%post -n libdevman
/sbin/ldconfig

%postun -n libdevman
/sbin/ldconfig

%files -n libdevman-devel
%{_includedir}/devman/devman.h
%{_includedir}/devman/devman_image.h
%{_includedir}/devman/devman_managed.h
%{_includedir}/devman/devman_haptic.h
%{_includedir}/devman/devman_PG.h
%{_libdir}/pkgconfig/devman.pc
%{_libdir}/libdevman.so

%post -n libdevman-devel
/sbin/ldconfig

%postun -n libdevman-devel
/sbin/ldconfig

%files -n libdevman-haptic-devel
%{_includedir}/devman/devman_haptic_ext.h
%{_includedir}/devman/devman_haptic_ext_core.h
%{_libdir}/pkgconfig/devman_haptic.pc

%files -n libdeviced
%defattr(-,root,root,-)
%{_libdir}/libdeviced.so.*
%manifest deviced.manifest

%post -n libdeviced
/sbin/ldconfig

%postun -n libdeviced
/sbin/ldconfig

%files -n libdeviced-devel
%defattr(-,root,root,-)
%{_includedir}/deviced/dd-battery.h
%{_libdir}/libdeviced.so
%{_libdir}/pkgconfig/deviced.pc

%post -n libdeviced-devel
/sbin/ldconfig

%postun -n libdeviced-devel
/sbin/ldconfig
