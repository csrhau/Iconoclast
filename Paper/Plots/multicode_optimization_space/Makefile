TEX = lualatex -shell-escape 

.PHONY: all view

all : multicode.pdf multicode.png

clean: 
	rm -rf *.auxlock
	rm -rf *.aux
	rm -rf *.pdf
	rm -rf *.dpth
	rm -rf *.dep
	rm -rf *.log
	rm -rf *.png

multicode.pdf : multicode.tex
	$(TEX) multicode.tex

multicode.png: multicode.pdf
	convert -verbose multicode-figure0.pdf $@
  
