Name:       system-server
Summary:    System server
Version:    0.1.51
Release:    1
Group:      TO_BE/FILLED_IN
License:    Flora Software License
Source0:    system-server-%{version}.tar.gz
Source1:    system-server.service
Source1001: packaging/system-server.manifest 
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
cp %{SOURCE1001} .
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants
install -m 0644 %{SOURCE1} %{buildroot}%{_libdir}/systemd/system/system-server.service
ln -s ../system-server.service %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/system-server.service

mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc5.d/
ln -s %{_sysconfdir}/init.d/system_server.sh %{buildroot}%{_sysconfdir}/rc.d/rc5.d/S00system-server


%preun
if [ $1 == 0 ]; then
    systemctl stop system-server.service
fi

%post 
systemctl daemon-reload
if [ $1 == 1 ]; then
    systemctl restart system-server.service
fi

vconftool set -t int memory/Battery/Charger -1 -i
vconftool set -t int memory/Battery/Status/Low -1 -i
vconftool set -t int memory/Battery/Capacity -1 -i
vconftool set -t int memory/Device/EarJackKey 0 -i
vconftool set -t int db/system/timechange 0 -i
vconftool set -t int memory/sysman/low_memory 1 -i
vconftool set -t int memory/Connectivity/USB -1 -i
vconftool set -t int memory/Device/mmc_format 0 -i
vconftool set -t int memory/Device/Sliding_keyboard -1 -i

vconftool set -t int memory/Device/Mmc -1 -i
vconftool set -t int db/MainLCD/Backlight/Normal -1 -i
vconftool set -t int memory/Device/EarJack -1 -i
vconftool set -t int memory/Device/Cradle/Status -1 -i

vconftool set -t int memory/Device/usbhost/added_storage 0 -i
vconftool set -t int memory/Device/usbhost/removed_storage 0 -i

vconftool set -t string memory/Device/usbhost/added_storage_uevent "" -i
vconftool set -t string memory/Device/usbhost/removed_storage_uevent "" -i

vconftool set -t int memory/Device/usbhost/connect -1 -i

heynotitool set power_off_start

heynotitool set mmcblk_add
heynotitool set mmcblk_remove

heynotitool set device_usb_chgdet
heynotitool set device_ta_chgdet
heynotitool set device_earjack_chgdet
heynotitool set device_earkey_chgdet
heynotitool set device_tvout_chgdet
heynotitool set device_hdmi_chgdet
heynotitool set device_cradle_chgdet
heynotitool set device_charge_chgdet
heynotitool set device_keyboard_chgdet

mkdir -p /etc/udev/rules.d
if ! [ -L /etc/udev/rules.d/91-system-server.rules ]; then
        ln -s %{_datadir}/system-server/udev-rules/91-system-server.rules /etc/udev/rules.d/91-system-server.rules
fi

%postun
systemctl daemon-reload


%files 
%manifest system-server.manifest
%{_bindir}/system_server
%{_bindir}/restart
%{_bindir}/movi_format.sh
%{_bindir}/sys_event
%{_libdir}/systemd/system/multi-user.target.wants/system-server.service
%{_libdir}/systemd/system/system-server.service
%{_datadir}/system-server/udev-rules/91-system-server.rules
%config %{_sysconfdir}/dbus-1/system.d/system-server.conf
%{_sysconfdir}/rc.d/init.d/system_server.sh
%{_sysconfdir}/rc.d/rc5.d/S00system-server

