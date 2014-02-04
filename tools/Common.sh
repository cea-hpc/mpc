#!/bin/sh
######################################################
#            PROJECT  : svMPC                        #
#            VERSION  : 0.0.0                        #
#            DATE     : 11/2013                      #
#            AUTHOR   : Valat Sébastien              #
#            LICENSE  : CeCILL-C                     #
######################################################

######################################################
#Define some constants
INTERNAL_KEY='internal'
PREFIX='/usr/local'
MAKE_J=''
ALL_PACKAGES=''
ALL_INTERNALS='false'
ENABLE_COLOR='false'
GEN_DEP_HELPS='false'
SUBPREFIX=''


######################################################
#function to generate the prefix variable and set it to internal
#Args   :
#   -$1 : Module name
#Result : Create var
setModulesToInternal()
{
	local list=`cat "${PROJECT_SOURCE_DIR}/config.txt" | cut -f 1 -d ';' |  sed -e "s/^#[0-9A-Za-z_-\ #]*//g" | xargs echo`
	for module in $list; do
		genPrefix $module
		echo $module	
	done	
}

######################################################
#function to generate the prefix variable and set it to internal
#Args   :
#   -$1 : Module name
#Result : Create var
genPrefix()
{
	local name=`echo "${1}" | tr '[:lower:]' '[:upper:]' | sed -e "s/-/_/g"`
	eval "${name}_PREFIX=\"${INTERNAL_KEY}\""
}

######################################################
#function to search prefix of CMD in path
#Args   :
#   -$1 : Variable in witch to put the result
#   -$2 : Command to search
#Result : Set the result in $prefix
getPrefixOfCmd()
{
	#extract parameters
	local outvar="$1"
	local cmd="$2"
	local prefix=''

	#if not setup, need to find it, try by xml2-config in PATH
	if which ${cmd} 1>/dev/null 2>/dev/null; then
		#extract prefix by removin bin/cmd
		prefix=`which ${cmd} | xargs dirname | xargs dirname`
	fi
	
	#setup output
	eval "${outvar}=\"${prefix}\""
}

######################################################
#function to search prefix of CMD with pkg-config
#Args   :
#   -$1 : Variable in witch to put the result
#   -$2 : Package to search
#Result : Set the result in $prefix
getPrefixWithPkgConfig()
{
	#extract parameters
	local outvar="$1"
	local package="$2"
	local prefix=''

	#if not setup, need to find it, try by xml2-config in PATH
	if pkg-config "${package}" --exists 1>/dev/null 2>/dev/null; then
		#extract prefix by removin bin/cmd
		prefix=`pkg-config "${package}" '--variable=prefix'`
		#if prefix var is not set, try with include dir
		if [ -z "${prefix}" ]; then
			prefix=`pkg-config "${package}" '--variable=includedir' | xargs dirname`
		fi
	fi
	
	#setup output
	eval "${outvar}=\"${prefix}\""
}

######################################################
include()
{
	#setup local var
	local file="${PROJECT_SOURCE_DIR}/tools/${1}.sh"

	#check before include
	if [ -f "${file}" ]; then
		source "${PROJECT_SOURCE_DIR}/tools/${1}.sh"
	else
		echo "Missing file to include : ${file}" 1>&2
		exit 1
	fi
}

######################################################
#Extract parameter value and put in given variable
#Args :
# -$1 : Output variable name
# -$2 : parameter name (without --, eg. --with-XXX => 'with-XXX')
# -$3 : User value (--with-XXX=blablable)
extractParamValue()
{
	#extract in local vars
	local outputvar="$1"
	local name="$2"
	local arg="$3"

	value=`echo "$arg" | sed -e "s/^--${name}=//g"`
	eval "${outputvar}=\"${value}\""
}

######################################################
#Extract parameter value and put in given variable
#Args :
# -$1 : Output variable name
# -$2 : parameter prefix (eg --libxml2-XXXX to add --XXX to libxml2 params)
# -$3 : User value (--libxml2-XXXX)
extractAndAddPackageOption()
{
	#extract in local vars
	local outputvar="$1"
	local name="$2"
	local arg="$3"
	
	value=`echo "$arg" | sed -e "s/^--${name}//g"`
	eval "${outputvar}=\"\$${outputvar} ${value}\""
}

######################################################
#Enable use of prefix in env variables
# Args : 
#  -$1 : Prefix to use
enablePrefixEnv()
{
	#extract in local vars
	local prefix="$1"

	#do exports
	PATH="${prefix}/bin:${PATH}"
	LD_LIBRARY_PATH="${prefix}/lib:${LD_LIBRARY_PATH}"
	PKG_CONFIG_PATH="${prefix}/lib/pkg-config/:${prefix}/share/pkg-config/:${PKG_CONFIG_PATH}"
}

######################################################
#Apply on template
# Args :
# - $1 : Template file to use
applyOnTemplate()
{
	#extract args
	local file="$1"

	#Check if the file exist
	if [ ! -f "$file" ]; then
		echo "Invalid file to use as template : $file" 1>&2
		exit 1
	fi

	#Search list of vars to replace
	varList=`egrep -o "@[0-9A-Za-z_-]*@" "${file}" | sort -u | sed -e 's/^@//' '-e s/@$//'`

	#Generate sed remplacement string
	sedOpts=''
	for var in $varList; do
		eval "value=\${${var}}"
		sedOpts="${sedOpts} -e 's#@${var}@#${value}#g'"
	done
	
	#apply ??? why didn't work without evel ???
	eval "sed ${sedOpts} \"${file}\"" || exit 1
}

######################################################
registerPackage()
{
	ALL_PACKAGES="${ALL_PACKAGES} ${1}"
}

######################################################
#Generate makefile rules do build package
# Args :
#  -$1 : package name (eg. libxml2),
#  -$2 : variable prefix (eg LIBXML2_PREFIX and check if equal to internal one)
#  -$3 : [OPTIONAL] template file to use.
setupInstallPackage()
{
	#made local
	local name="$1"
	local varprefix="$2"
	local template="$3"
	
	#if template is empty, use default
	if [ -z "$template" ]; then
		template="${PROJECT_TEMPLATE_DIR}/Makefile.${name}.in"
	fi
	
	#extract prefix target
	eval "prefix=\${${varprefix}_PREFIX}"
	
	#Load package options
	local version=`cat "${PROJECT_SOURCE_DIR}/config.txt" | grep "^${name} " | cut -f 2 -d ';' | xargs echo`
	local status=`cat "${PROJECT_SOURCE_DIR}/config.txt" | grep "^${name} " | cut -f 3 -d ';' | xargs echo`
	local deps=`cat "${PROJECT_SOURCE_DIR}/config.txt" | grep "^${name} " | cut -f 4 -d ';' | xargs echo`
	local options=`cat "${PROJECT_SOURCE_DIR}/config.txt" | grep "^${name} " | cut -f 6 -d ';'`
	
	#extract user options
	eval "userOptions=\"\$${varprefix}_BUILD_PARAMETERS\""
	
	#add dep if want to gen help
	if [ "$GEN_DEP_HELPS" = 'true' ]; then deps="${deps} ${name}-gen-help"; fi

	#check if need to do it
	if [ "$prefix" = "${INTERNAL_KEY}" ]; then
		PACKAGE_DEPS="$deps"
		PACKAGE_VAR_NAME="$varprefix"
		PACKAGE_OPTIONS="$options"
		PACKAGE_NAME="${name}"
		PACKAGE_VERSION="${version}"
		USER_PARAMETERS="$userOptions"
		applyOnTemplate "${template}"
	else
		echo "${name}:"
	fi

	#Line to split rules
	echo ''
	
	#register to list
	if [ "$status" = 'install' ]; then
		registerPackage "${name}"
	fi
}

######################################################
installAutotoolsExternPackage()
{
	setupInstallPackage "$1" "$2" "${PROJECT_TEMPLATE_DIR}/Makefile.generic.autotools.in"
}

######################################################
installAutotoolsLocalPackage()
{
	setupInstallPackage "$1" "$2" "${PROJECT_TEMPLATE_DIR}/Makefile.local.autotools.in"
}

######################################################
genAbstractMultiArchBinDir()
{
	mkdir -p "$PREFIX/generic/bin" || exit 1
	cp ${PROJECT_SOURCE_DIR}/arch_wrapper "$PREFIX/generic/bin/"

	for exe in $PREFIX/host/bin/*; do
		ln -sf $PREFIX/generic/bin/arch_wrapper $PREFIX/generic/bin/`basename "$exe"`
	done
}

######################################################
showHelp()
{
	cat "${PROJECT_SOURCE_DIR}/tools/help/global.txt"
	exit 1
}

######################################################
fatal()
{
	echo "$*" 1>&2
	exit 1
}
