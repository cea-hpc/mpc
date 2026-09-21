#
# SPEC File
# Template
# Don't forget:
# - Uppercase letter to the to of each line
# - The space after keyword !!!
#

Summary:
#project name (package name)
Name:
Version:
Release:
License:
Group: Applications
Source:
URL:
Distribution:
# Name <mail>
Packager:
#if need, add new dependencies to build
BuildRequires:
#if need add new dependencies to execution
Requires:
#patches declarations (if needed)
#patch${i}: patch_name.patch

#sub-packages creation (if needed). You can do as much as you want
#%package <name_package>
#Requires:
#Group: Applications
#Summary:

#main package description
%description
#comment

#sub-package description (if needed)
#%description <name_package>
#comment

#define module version to compile with the package
#%define module_version	${version}

#Two lines required
%prep
%setup
#patches building (if needed). -p1 option means the patch is located in an adjacent folder to the current
#%patch${i} -p1

#build directives
%build
cmake -B BUILD
cmake --build BUILD

#install directives
%install
DESTDIR=$RPM_BUILD_ROOT cmake --install BUILD

#clean directives
%clean

#installation description
# main package
%files
#the line below means:
#Set the default rights:
# - : binary rights (ex: 777, 664,...)
# root : file owner
# root : group owner
# -    : deprecated option -> let the hyphen
%defattr(-,root,root,-)
/usr/local/...

#sub-packages
#%files <name_package>
#%defattr(-,root,root,-)

#lines required
%doc README_TEMPLATE.pdf

%changelog
