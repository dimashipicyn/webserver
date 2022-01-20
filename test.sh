#!/bin/bash
echo '<!DOCTYPE html>'
echo '<html>'
echo '<head>'
echo '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">'
echo '<title>Test CGI. Shows list of environment variables</title>'
echo '</head>'
echo '<body>'
env
echo '</body>'
echo '</html>'

exit 0
