TEX = lualatex -shell-escape 

.PHONY: all view

all : freqplot.pdf freqplot.png

clean: 
	rm -rf *.auxlock
	rm -rf *.aux
	rm -rf *.pdf
	rm -rf *.dpth
	rm -rf *.dep
	rm -rf *.log
	rm -rf *.png

freqplot.pdf : freqplot.tex
	$(TEX) freqplot.tex

freqplot.png: freqplot.pdf
	convert -verbose freqplot-figure0.pdf $@
  
