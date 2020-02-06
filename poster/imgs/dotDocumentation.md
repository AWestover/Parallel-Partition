$ dot2tex foo.gv -f tikz > foo.tex
$ pdflatex foo.tex

digraph G {
  a_1 [texlbl="$X_{1}^{(1)}$"];
  a_2 [texlbl="$X_{1}^{(2)}$"];
  a_3 [texlbl="$X_{1}^{(3)}$"];
  a_1-> a_2 -> a_3 -> a_1;
}
