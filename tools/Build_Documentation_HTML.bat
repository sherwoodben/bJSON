@echo off
echo cleaning old output...
rmdir /s /q "../docs"
echo running doxygen...
CALL doxygen Doxyfile > NUL
cd "../docs"
type NUL > .nojekyll
echo opening html...
START "" "index.html"
PAUSE