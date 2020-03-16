all:

pdf: userman.pdf devman.pdf

userman.pdf: cleanall userman.tex devman.tex
	pdflatex userman
	makeindex userman.idx
	pdflatex userman
	latex_count=5 ; \
	while egrep -s 'Rerun (LaTeX|to get cross-references right)' userman.log && [ $$latex_count -gt 0 ] ;\
	    do \
	      echo "Rerunning latex...." ;\
	      pdflatex userman ;\
	      latex_count=`expr $$latex_count - 1` ;\
	    done

devman.pdf: clean devman.tex
	pdflatex devman
	makeindex devman.idx
	pdflatex devman
	latex_count=5 ; \
	while egrep -s 'Rerun (LaTeX|to get cross-references right)' devman.log && [ $$latex_count -gt 0 ] ;\
	    do \
	      echo "Rerunning latex...." ;\
	      pdflatex devman ;\
	      latex_count=`expr $$latex_count - 1` ;\
	    done

cleanall:
	rm -f *.ps *.dvi *.aux *.toc *.idx *.ind *.ilg *.log *.out *.brf *.blg *.bbl userman.pdf devman.pdf

clean:
	rm -f *.ps *.dvi *.aux *.toc *.idx *.ind *.ilg *.log *.out *.brf *.blg *.bbl
