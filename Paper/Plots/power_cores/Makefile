TEX = lualatex -shell-escape 

.PHONY: all view

all : powercores.pdf powercores.png

clean: 
	rm -rf *.auxlock
	rm -rf *.aux
	rm -rf *.pdf
	rm -rf *.dpth
	rm -rf *.dep
	rm -rf *.log
	rm -rf *.png

powercores.pdf : powercores.tex
	$(TEX) powercores.tex

powercores.png: powercores.pdf
	convert -verbose powercores-figure0.pdf $@
  
