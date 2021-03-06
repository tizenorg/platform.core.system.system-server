#sbs-git:slp/pkgs/s/system-server system-server 0.1.51 56e16bca39f96d6c8aed9ed3df2fea9b393801be
Name:       system-server
Summary:    System server
Version: 0.1.51
Release:    1
Group:      TO_BE/FILLED_IN
License:    Flora Software License
Source0:    system-server-%{version}.tar.gz
Requires(post): /usr/bin/vconftool
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

%description
Description: System server


%prep
%setup -q
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}

%build
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc3.d/
ln -s %{_sysconfdir}/init.d/system_server.sh %{buildroot}%{_sysconfdir}/rc.d/rc3.d/S35system-server
mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc5.d/
ln -s %{_sysconfdir}/init.d/system_server.sh %{buildroot}%{_sysconfdir}/rc.d/rc5.d/S00system-server

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
heynotitool set device_keyboard_add
heynotitool set device_keyboard_remove
heynotitool set device_mouse_add
heynotitool set device_mouse_remove
heynotitool set device_unknown_usb_add
heynotitool set device_unknown_usb_remove
heynotitool set device_camera_add
heynotitool set device_camera_remove


mkdir -p /etc/udev/rules.d
if ! [ -L /etc/udev/rules.d/91-system-server.rules ]; then
        ln -s %{_datadir}/system-server/udev-rules/91-system-server.rules /etc/udev/rules.d/91-system-server.rules
fi


%files
%{_bindir}/system_server
%{_bindir}/restart
%{_bindir}/movi_format.sh
%{_bindir}/sys_event
%{_bindir}/sys_device_noti
%{_datadir}/system-server/sys_device_noti/batt_full_icon.png
%{_datadir}/system-server/udev-rules/91-system-server.rules
%{_sysconfdir}/rc.d/init.d/system_server.sh
%{_sysconfdir}/rc.d/rc3.d/S35system-server
%{_sysconfdir}/rc.d/rc5.d/S00system-server
