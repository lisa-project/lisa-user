Summary: Network Multilayer Switching on Linux
Name: lisa
Version: %{lisa_version}_%{kernel_version}
Release: 1%{?dist}
Group: Applications/System
Source: http://lisa.ines.ro/download/tarball/lisa-%{lisa_version}.tar.gz
URL:    http://lisa.ines.ro

License: GPL
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Requires: kernel = %{kernel_version}
BuildRequires: ncurses-devel readline-devel

%description
LISA stands for LInux Switching Appliance. It's a project that aims at
building a network multilayer switching device that runs on a (slightly
modified) PC hardware architecture running Linux.

The device should be able to perform Layer 2 and Layer 3 packet
switching and run dynamic routing protocols such as OSPF and BGP. The
configuration and operation command line interface will be similar to
Cisco's IOS.

%package -n liblisa
Summary: Libraries for lisa userspace operation.
License: GPL
Group: System Environment/Libraries

%description -n liblisa
This package contains dynamic libraries required to run most of the
lisa binaries.

%package xen
Summary: Xen integration files.
License: GPL
Group: Applications/System
Requires: lisa

%description xen
This package contains the files required for xen integration.

%package embedded
Summary: Embedded/dedicated system files.
License: GPL
Group: Applications/System
Requires: lisa

%description embedded
This package contains the files required for operation on an embeddedd
or dedicated system. It is not intended for a standard linux box.

In contains such binaries as login and reboot and will most likely
conflict with standard linux utility packages.

%prep
%setup -n lisa

%build
make user DIST=1

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

make install ROOT=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%post
chkconfig --add lisa

%preun
[ -f /var/lock/subsys/lisa ] && service lisa stop
chkconfig --del lisa

%post -n liblisa -p /sbin/ldconfig

%postun -n liblisa -p /sbin/ldconfig

%files
%defattr(-,root,root)
%{_sbindir}/cdpd
/bin/filter
%{_sbindir}/swcfgload
%{_sbindir}/swcli
%{_sbindir}/swclid
%{_sbindir}/swcon
%{_sbindir}/swlogin
%{_sbindir}/swctl
%{_sysconfdir}/rc.d/init.d/lisa
%{_sysconfdir}/lisa
%config(noreplace) %{_sysconfdir}/lisa/config.text

%files -n liblisa
%defattr(-,root,root)
%{_libdir}/libswcli.so

%files xen
%defattr(-,root,root)
%{_sysconfdir}/xen/scripts/network-lisa
%{_sysconfdir}/xen/scripts/vif-lisa

%files embedded
%defattr(-,root,root)
/bin/login
/bin/reboot

%changelog
* Tue Feb 19 2008 Radu Rendec <radu.rendec@ines.ro> - 2008.02.19_2.6.21-1
- Fix packaging of lisa config dirs and lisa system service setup

* Mon Feb 11 2008 Radu Rendec <radu.rendec@ines.ro> - 2008.02.11_2.6.21-1
- Initial Fedora package
