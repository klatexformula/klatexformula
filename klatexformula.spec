# Spec file for KLatexFormula

Name: klatexformula
%define kde_path /opt/kde3
Version: 2.0.0alpha4
Release: 00
Vendor: Philippe Faist
Summary: A KDE Application to easily create an image from a LaTeX equation that you can save, copy or drag-and-drop
Group: Application/Utilities
Packager: Philippe Faist
%define install_dir %{_tmppath}/%{name}-root
BuildRoot:  %{install_dir}
Source: klatexformula-%{version}.tar.gz
Requires: qt3 >= 3.1.0, kdelibs3 >= 3.1.0, te_latex, gs
License: GPL


%description
This KDE application is especially designed for generating an image (e.g. PNG) from a LaTeX equation, this using a nice GUI.
Support for Copy-Paste, Save as image/pdf/ps and drag-and-drop is included.
In addition to supporting a whole GUI-based user interface, this package includes the library KLFBackend to integrate KLatexFormula
functionality into your programs.

%prep
%setup

%build
CFLAGS="$RPM_OPT_FLAGS" CXXFLAGS="$RPM_OPT_FLAGS" ./configure --host=i686-suse-linux --build=i686-suse-linux --target=i586-suse-linux --program-prefix= --prefix=%kde_path --sysconfdir=/etc --disable-debug
make

%install
rm -rf %{buildroot}
%makeinstall

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig
%files
%defattr(-, root, root)
%doc AUTHORS COPYING ChangeLog NEWS README TODO
%{kde_path}/bin/*
%{kde_path}/share/apps/%{name}
%{kde_path}/share/applnk/Utilities/*
%{kde_path}/share/icons/*/*/*/*
%{kde_path}/share/doc/HTML/*/*
#%{kde_path}/*

%changelog

