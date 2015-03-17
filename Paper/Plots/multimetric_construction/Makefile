TEX = lualatex -shell-escape 

.PHONY: all view

all : plot.pdf

clean: 
	rm -rf *.auxlock
	rm -rf *.aux
	rm -rf *.pdf
	rm -rf *.dpth
	rm -rf *.dep
	rm -rf *.log
	rm -rf *.png

plot.pdf : plot.tex plot_core.tex
	$(TEX) plot.tex

