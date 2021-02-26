# Copyright (c) 2019-2020 Info@MediaArea.net
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.

# norootforbuild

%global dvrescue_version		0.20.11
%global libmediainfo_version	20.09
%global libzen_version			0.4.38

Name:			dvrescue
Version:		%dvrescue_version
Release:		1
Summary:		Convert DV tapes into digital files suitable for long-term preservation
Group:			Productivity/Multimedia/Other
License:		BSD-3-Clause
URL:			https://MediaArea.net/DVRescue
Packager:		Jerome Martinez <Info@MediaArea.net>
Source0:		dvrescue_%{version}.tar.gz
Prefix:			%{_prefix}
BuildRoot:		%{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires: 	gcc-c++
BuildRequires:	pkgconfig
BuildRequires:  automake
BuildRequires:  autoconf
BuildRequires:  libtool
BuildRequires:	libmediainfo-devel >= %libmediainfo_version
BuildRequires:	libzen-devel >= %libzen_version
BuildRequires:	zlib-devel
Requires:		xmlstarlet

%if 0%{?rhel_version} >= 800 || 0%{?centos_version} >= 800
BuildRequires:  gdb
%endif

%if 0%{?mageia}
%ifarch x86_64
BuildRequires:  lib64openssl-devel
%else
BuildRequires:  libopenssl-devel
%endif
%endif

%package gui
Summary:	Convert DV tapes into digital files suitable for long-term preservation (GUI)
Group:		Productivity/Multimedia/Other

BuildRequires:  nasm
BuildRequires:  libXv-devel
%if 0%{?fedora_version} || 0%{?centos} >= 7
BuildRequires:  pkgconfig(Qt5)
BuildRequires:  pkgconfig(Qt5QuickControls2)
BuildRequires:  pkgconfig(Qt5Svg)
BuildRequires:  pkgconfig(Qt5XmlPatterns)
BuildRequires:  pkgconfig(Qt5Qwt6)
BuildRequires:  pkgconfig(alsa)
%endif

%if 0%{?mageia}
%ifarch x86_64
BuildRequires:  lib64qt5base5-devel
BuildRequires:  lib64qt5quicktemplates2-devel
BuildRequires:  lib64qt5quicktemplates2_5
BuildRequires:  lib64qt5quickcontrols2-devel
BuildRequires:  lib64qt5quickcontrols2_5
BuildRequires:  lib64qt5quickwidgets-devel
BuildRequires:  lib64qt5svg-devel
BuildRequires:  lib64qt5xmlpatterns-devel
BuildRequires:  lib64qt5xmlpatterns5
BuildRequires:  lib64qwt-qt5-devel
BuildRequires:  lib64qwt-qt5_6
%else
BuildRequires:  libqt5base5-devel
BuildRequires:  libqt5quicktemplates2-devel
BuildRequires:  libqt5quicktemplates2_5
BuildRequires:  libqt5quickcontrols2-devel
BuildRequires:  libqt5quickcontrols2_5
BuildRequires:  libqt5quickwidgets-devel
BuildRequires:  libqt5svg-devel
BuildRequires:  libqt5xmlpatterns-devel
BuildRequires:  libqt5xmlpatterns5
BuildRequires:  libqwt-qt5-devel
BuildRequires:  libqwt-qt5_6
%endif
%endif

%if 0%{?suse_version} >= 1200
BuildRequires:  libqt5-qtbase-devel
BuildRequires:  libqt5-qtsvg-devel
BuildRequires:  libqt5-qtxmlpatterns-devel
BuildRequires:  libQt5QuickControls2-devel
BuildRequires:  qwt6-devel
%endif

%description
Data migration from DV tapes into digital files suitable for long-term preservation

%description gui
Data migration from DV tapes into digital files suitable for long-term preservation (Graphical User Interface)

%prep
%setup -q -n dvrescue
%__chmod 644 dvrescue/*.txt dvrescue/*.md dvrescue/LICENSE.txt

%build
export CFLAGS="-g $RPM_OPT_FLAGS"
export CXXFLAGS="-g $RPM_OPT_FLAGS"

# build CLI
pushd dvrescue/Project/GNU/CLI
	%__chmod +x autogen
	./autogen
	%if 0%{?mageia} >= 6
		%configure --disable-dependency-tracking
	%else
		%configure
	%endif

	%__make %{?jobs:-j%{jobs}}
popd

# now build GUI

pushd ffmpeg
	./configure --enable-gpl --disable-autodetect --enable-alsa --disable-doc --disable-programs --disable-debug --enable-pic --enable-static --enable-lto --disable-shared --prefix=`pwd`
	%__make %{?jobs:-j%{jobs}} install
popd
mkdir -p dvrescue/Source/GUI/dvrescue/build
pushd dvrescue/Source/GUI/dvrescue/build
	export USE_SYSTEM=true
	qmake-qt5 ..
	%__make %{?jobs:-j%{jobs}}
popd


%install
pushd dvrescue/Project/GNU/CLI
	%__make install DESTDIR=%{buildroot}
popd

pushd dvrescue/Source/GUI/dvrescue/build
	%__install -D -m 755 dvrescue/dvrescue %{buildroot}%{_bindir}/dvrescue-gui
popd
%__install -D -m 644 dvrescue/Source/GUI/dvrescue/dvrescue/icons/icon.png %{buildroot}%{_datadir}/pixmaps/dvrescue.png
%__install -D -m 644 dvrescue/Project/GNU/GUI/dvrescue-gui.desktop %{buildroot}/%{_datadir}/applications/dvrescue-gui.desktop
%__install -D -m 644 dvrescue/Project/GNU/GUI/dvrescue-gui.metainfo.xml %{buildroot}%{_datadir}/metainfo/dvrrescue-gui.metainfo.xml

%clean
[ -d "%{buildroot}" -a "%{buildroot}" != "" ] && %__rm -rf "%{buildroot}"

%files
%defattr(-,root,root,-)
%doc dvrescue/LICENSE.txt
%doc dvrescue/History.txt
%{_bindir}/dvrescue
%{_bindir}/dvloupe
%{_bindir}/dvmap
%{_bindir}/dvpackager
%{_bindir}/dvplay
%{_bindir}/dvsampler

%files gui
%defattr(-,root,root,-)
%doc dvrescue/LICENSE.txt
%doc dvrescue/History.txt
%{_bindir}/dvrescue-gui
%{_datadir}/pixmaps/*.png
%{_datadir}/applications/*.desktop
%{_datadir}/metainfo/*.xml

%changelog
* Tue Jan 01 2019 Jerome Martinez <Info@MediaArea.net> - 0.20.11-0
- See History.txt for more info and real dates
