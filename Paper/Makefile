MAIN_FILES := $(wildcard *.tex)
MAIN_FILES += library.bib

LFLAGS := -interaction=nonstopmode

main: $(MAIN_FILES)
	rm -f *.log *.aux *.pdf *.bbl *.blg
	pdflatex $(LFLAGS) main.tex
	bibtex main.aux
	pdflatex $(LFLAGS) main.tex
	pdflatex $(LFLAGS) main.tex

.PHONY: miniclean
miniclean:
	@echo $(TEX_FILES)
	rm -f *.log *.aux *.pdf *.bbl *.blg

.PHONY: clean
clean: miniclean

