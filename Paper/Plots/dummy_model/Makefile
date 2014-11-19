TEX = lualatex -shell-escape 

.PHONY: all view

all : dummymodel.pdf dummymodel.png

clean: 
	rm -rf *.auxlock
	rm -rf *.aux
	rm -rf *.pdf
	rm -rf *.dpth
	rm -rf *.dep
	rm -rf *.log
	rm -rf *.png

dummymodel.pdf : dummymodel.tex
	$(TEX) dummymodel.tex

dummymodel.png: dummymodel.pdf
	convert -verbose -quality 100 dummymodel-figure0.pdf $@
  
