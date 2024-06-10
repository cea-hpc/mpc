#
# Rule RPM Package generation
#
rpm:
	@(cd ${TOP_SOURCE_DIR}/packaging/rpm-package-generator && ./rpm_package_generator -s=auto --output=$(PWD))
#
# Rule SRPM package generation
#
srpm:

	@(cd ${TOP_SOURCE_DIR}/packaging/rpm-package-generator && ./rpm_package_generator -s=auto --srpm --output=$(PWD))
