TEX = lualatex -shell-escape 

.PHONY: all view

all : coresfreq.pdf coresfreq.png

clean: 
	rm -rf *.auxlock
	rm -rf *.aux
	rm -rf *.pdf
	rm -rf *.dpth
	rm -rf *.dep
	rm -rf *.log
	rm -rf *.png

coresfreq.pdf : coresfreq.tex
	$(TEX) coresfreq.tex

coresfreq.png: coresfreq.pdf
	convert -verbose coresfreq-figure0.pdf $@
  
