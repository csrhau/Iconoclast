TEX = lualatex -shell-escape 

.PHONY: all view

all : optspace.pdf optspace.png

clean: 
	rm -rf *.auxlock
	rm -rf *.aux
	rm -rf *.pdf
	rm -rf *.dpth
	rm -rf *.dep
	rm -rf *.log
	rm -rf *.png

optspace.pdf : optspace.tex
	$(TEX) optspace.tex

optspace.png: optspace.pdf
	convert -verbose optspace-figure0.pdf $@
  
