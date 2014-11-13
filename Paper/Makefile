MAIN_FILES := $(wildcard *.tex)
MAIN_FILES += library.bib

LFLAGS := -interaction=nonstopmode

main: PLOTS $(MAIN_FILES)
	rm -f *.log *.aux *.pdf *.bbl *.blg
	pdflatex $(LFLAGS) main.tex
	bibtex main.aux
	pdflatex $(LFLAGS) main.tex
	pdflatex $(LFLAGS) main.tex

PLOTS: FORCE
	for plot in Frequency tlp_vs_ilp ; do \
	  make -C Plots/$$plot ; \
	done

PLOTS_CLEAN: FORCE
	for plot in Frequency ; do \
	  make clean -C Plots/$$plot ; \
	done
    
FORCE: 

.PHONY: miniclean
miniclean:
	@echo $(TEX_FILES)
	rm -f *.log *.aux *.pdf *.bbl *.blg

.PHONY: clean
clean: PLOTS_CLEAN miniclean

