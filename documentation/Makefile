REP= lcd4linux drivers plugins

default: makefiles html
	true

makefiles: 
	@for i in $(REP) ; do \
  		(cd $$i && ln -sf ../Makefile.generic Makefile) ; \
	done

cleanmakefiles:
	@for i in $(REP) ; do \
 		rm -f $$i/Makefile ; \
	done

totalclean:
	@for i in $(REP) ; do \
		(cd $$i && make clean ; rm Makefile) ; \
	done
	rm -rf HTML

#catalog: catalogue.xml xsl/catalogue.xsl
#	test -d catalog || mkdir catalog
#	xsltproc --param dir \'$(PWD)\' \
#		 --param odir \'$(PWD)/catalog\' \
#	    xsl/catalogue.xsl catalogue.xml >catalog/index.html
#	cp charte/default/default.css catalog/

%:
	@for i in $(REP) ; do \
		(cd $$i && make $@) ; \
	done
