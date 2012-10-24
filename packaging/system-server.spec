#sbs-git:slp/pkgs/s/system-server system-server 0.1.51 56e16bca39f96d6c8aed9ed3df2fea9b393801be
Name:       system-server
Summary:    System server
Version: 0.1.52
Release:    1
Group:      TO_BE/FILLED_IN
License:    Flora Software License
Source0:    system-server-%{version}.tar.gz
Source1:    system-server.service
Source2:    system-server.manifest
BuildRequires:  cmake
BuildRequires:  libattr-devel
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(heynoti)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(sysman)
BuildRequires:  pkgconfig(tapi)
BuildRequires:  pkgconfig(devman)
BuildRequires:  pkgconfig(pmapi)
BuildRequires:  pkgconfig(edbus)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(syspopup-caller)
BuildRequires:  pkgconfig(devman_plugin)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(svi)
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(usbutils)
BuildRequires:	gettext
Requires(preun): /usr/bin/systemctl
Requires(post): /usr/bin/systemctl
Requires(post): /usr/bin/vconftool
Requires(postun): /usr/bin/systemctl

%description
Description: System server


%prep
%setup -q
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}

%build
cp %{SOURCE2} .
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc3.d/
ln -s %{_sysconfdir}/init.d/system_server.sh %{buildroot}%{_sysconfdir}/rc.d/rc3.d/S35system-server
mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc5.d/
ln -s %{_sysconfdir}/init.d/system_server.sh %{buildroot}%{_sysconfdir}/rc.d/rc5.d/S00system-server

mkdir -p %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants
install -m 0644 %{SOURCE1} %{buildroot}%{_libdir}/systemd/system/system-server.service
ln -s ../system-server.service %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/system-server.service
%post

vconftool set -t int memory/sysman/usbhost_status -1 -i
vconftool set -t int memory/sysman/mmc -1 -i
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

vconftool set -t string memory/private/sysman/added_storage_uevent "" -i
vconftool set -t string memory/private/sysman/removed_storage_uevent "" -u 5000 -i


heynotitool set power_off_start

heynotitool set mmcblk_add
heynotitool set mmcblk_remove

heynotitool set device_usb_chgdet
heynotitool set device_ta_chgdet
heynotitool set device_earjack_chgdet
heynotitool set device_earkey_chgdet
heynotitool set device_tvout_chgdet
heynotitool set device_hdmi_chgdet
heynotitool set device_charge_chgdet
heynotitool set device_keyboard_chgdet
heynotitool set device_usb_host_add
heynotitool set device_usb_host_remove


mkdir -p /etc/udev/rules.d
if ! [ -L /etc/udev/rules.d/91-system-server.rules ]; then
        ln -s %{_datadir}/system-server/udev-rules/91-system-server.rules /etc/udev/rules.d/91-system-server.rules
fi

systemctl daemon-reload
if [ $1 == 1 ]; then
    systemctl restart system-server.service
fi


%preun
if [ $1 == 0 ]; then
    systemctl stop system-server.service
fi

%postun
systemctl daemon-reload


%files
%manifest system-server.manifest
%{_bindir}/system_server
%{_bindir}/restart
%{_bindir}/movi_format.sh
%{_bindir}/sys_event
%{_bindir}/sys_device_noti
%{_datadir}/system-server/sys_device_noti/batt_full_icon.png
%{_libdir}/systemd/system/multi-user.target.wants/system-server.service
%{_libdir}/systemd/system/system-server.service
%{_datadir}/system-server/udev-rules/91-system-server.rules
%{_datadir}/system-server/sys_device_noti/res/locale/*/LC_MESSAGES/*.mo
%config %{_sysconfdir}/dbus-1/system.d/system-server.conf
%{_sysconfdir}/rc.d/init.d/system_server.sh
%{_sysconfdir}/rc.d/rc3.d/S35system-server
%{_sysconfdir}/rc.d/rc5.d/S00system-server
