#FILENAME:      Makefile
#USAGE:         Top synApps Makefile
#Version:       $Revision: 19130 $
#Modified By:   $Author: mooney $
#Last Modified: $Date: 2015-03-13 15:44:50 -0500 (Fri, 13 Mar 2015) $
#HeadURL:       $URL: https://subversion.xray.aps.anl.gov/synApps/support/tags/synApps_5_8/Makefile $

#NOTES
#     - The "MODULE_LIST" order is based on compile time dependencies.
#     - The user must modify SUPPORT and EPICS_BASE in the
#       <synApps>/support/configure directory for the local configuration.
#     - To support multiple configurations, use multiple configure* directories
#     - Support modules can be shared between configurations only if
#       dependencies are not violated.  Only the "DIRS" are the target of
#       gnumake.  If this configuration is using a support module built by
#       another configuration, then the SUPPORT_DIRS line for that support
#       module must be commented out (i.e, must begin with a '#').
#     - To remove modules from the build, delete or comment out the module
#       in the <synApps>/configure/RELEASE file; not here.

# Note the only dependencies that matter in $(<module>)_DEPEND_DIRS are
# compile-time dependencies.

TOP = .

MASTER_FILE = $(TOP)/configure/RELEASE

include $(TOP)/configure/CONFIG

DIRS := $(DIRS) $(filter-out $(DIRS), configure)

define  MODULE_defined
  ifdef $(1)
  SUPPORT_DIRS  += $($(1))
  RELEASE_FILES += $($(1))/configure/RELEASE
  endif  
endef


###### 1st Tier Support Modules - Only Depend on EPICS BASE ######

#MODULE_LIST =  ETHER_IP MCA_S55 DEVSNMP
MODULE_LIST =  ETHER_IP DEVSNMP CAPUTLOG
$(foreach mod, $(MODULE_LIST), $(eval $(call MODULE_defined,$(mod)) ))

################### End of Support-Modules #####################

DIRS = $(SUPPORT_DIRS)

ACTIONS += uninstall realuninstall distclean cvsclean

include $(TOP)/configure/RULES_TOP

release:
	echo SUPPORT=$(SUPPORT)
	echo ' '
	echo EPICS_BASE=$(EPICS_BASE)
	echo ' '
	echo MASTER_FILE=$(MASTER_FILE)
	echo ' '
	echo DIRS=$(DIRS)
	echo ' '
	echo RELEASE_FILES=$(RELEASE_FILES)
	echo ' '
	$(PERL) $(TOP)/configure/makeReleaseConsistent.pl $(SUPPORT) $(EPICS_BASE) $(MASTER_FILE) $(RELEASE_FILES)

