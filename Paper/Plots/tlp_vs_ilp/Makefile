TEX = lualatex -shell-escape 

.PHONY: all view

all : tlpilp.pdf tlpilp.png

clean: 
	rm -rf *.auxlock
	rm -rf *.aux
	rm -rf *.pdf
	rm -rf *.dpth
	rm -rf *.dep
	rm -rf *.log
	rm -rf *.png

tlpilp.pdf : tlpilp.tex
	$(TEX) tlpilp.tex

tlpilp.png: tlpilp.pdf
	convert -verbose tlpilp-figure0.pdf $@
  
