# Path
DIR = $(notdir $(PWD))
OUTPUT = ../HTML

ifeq "$(DIR)" "lcd4linux"
	DIR = 
endif

ifeq "$(DIR)" ""
	XSLTPROC = xsltproc
else
	XSLTPROC = xsltproc --stringparam class $(DIR) --stringparam root "../"
endif

XMLVALID = xmllint --noout --valid

NAMES=$(shell find . -name "*.xml" -exec basename {} .xml \;)

html: $(patsubst %.xml,%.html,$(wildcard *.xml))
html-forced: $(patsubst %.xml,%.html-forced,$(wildcard *.xml))

%.html: %.xml %.xml-check output
	$(XSLTPROC) $< > $(OUTPUT)/$(DIR)/$@

%.html-forced: %.xml output
	$(XSLTPROC) $< > $(OUTPUT)/$(DIR)/$(patsubst %.xml,%.html,$<)

check: $(patsubst %.xml, %.xml-check, $(wildcard *.xml))

%.xml-check: %.xml
	@echo "*** Validation of $<"
	$(XMLVALID) $< 

output:
	test -d $(OUTPUT) || `mkdir $(OUTPUT); cp ../data/doc.css $(OUTPUT); cp  -R ../data/images $(OUTPUT)`
	test -d $(OUTPUT)/$(DIR) || mkdir $(OUTPUT)/$(DIR)

clean:
	rm -f $(OUTPUT)/$(DIR)/*.html

clean-bak:
	rm -f *~ *.bak

help:
	@echo -e ""
	@echo -e "Usage :"
	@echo -e "-------"
	@echo -e "make  or  make html :  builds all possible html pages from xml files"
	@echo -e "make %.html :          builds the %.html page from %.xml"
	@echo -e ""
	@echo -e "make html-forced  :    builds all possible html pages from xml files (no validity check)"
	@echo -e "make %.html-forced :   builds the %.html page from %.xml (no validity check)"
	@echo -e ""
	@echo -e "make check :           checks the validity of all possible xml files"
	@echo -e "make %.xml-check :     checks the validity of %.xml"
	@echo -e ""
	@echo -e "make clean :           deletes all generated html pages"
	@echo -e "make clean-bak :       deletes *~ and *.bak files"
	@echo -e ""
