#!/bin/bash
###############################################################################
# (c) Copyright Hewlett-Packard Development Company, L.P., 2007
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of version 2 the GNU General Public License as
#   published by the Free Software Foundation.
#   
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#   
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
###############################################################################
# 
# PURPOSE:
# Verify the audit record and labeled output of a postscript print job
# embedded in this test is a simple postscript file and the expected
# postscript output.  The test creates a socket printer and prints the file,
# capturing the output with netcat.  The test checks that the ouput matches
# and that the job was audited.


source tp_print_functions.bash || exit 2

# setup
# start by creating the needed files

INFILE=hello-world.ps
cat > $INFILE << EOF
%!
% Sample of printing text

/Times-Roman findfont	% Get the basic font
20 scalefont		% Scale the font to 20 points
setfont			% Make it the current font

newpath			% Start a new path
72 72 moveto		% Lower left corner of text at (72, 72)
(Hello, world!) show    % Typeset "Hello, world!"

showpage
EOF

LABELED=hello-label.ps
# labeled output only matches correctly labeled output with:
# printer of tests 
# job-id  of 42
# user    of root
# file    of hello-world.ps
cat > $LABELED << EOF
%!PS-Adobe-3.0
%%LanguageLevel: 1
%%DocumentData: Clean7Bit
%%DocumentSuppliedResources: procset bannerprint/1.0
%%DocumentNeededResources: font Helvetica Helvetica-Bold Times-Roman
%%Creator: Michael Sweet, Easy Software Products
%%CreationDate: May 10, 2000
%%Title: Test Page
%%For: (root)
%RBINumCopies: 1
%%Pages: (atend)
%%BoundingBox: (atend)
%%EndComments
%%BeginProlog
userdict/ESPshowpage/showpage load put
userdict/showpage{}put
%%BeginResource procset bannerprint 1.1 0
%
%   PostScript banner page for the Common UNIX Printing System ("CUPS").
%
%   Copyright 1993-2005 by Easy Software Products
%
%   These coded instructions, statements, and computer programs are the
%   property of Easy Software Products and are protected by Federal
%   copyright law.  Distribution and use rights are outlined in the file
%   "LICENSE.txt" which should have been included with this file.  If this
%   file is missing or damaged please contact Easy Software Products
%   at:
%
%       Attn: CUPS Licensing Information
%       Easy Software Products
%       44141 Airport View Drive, Suite 204
%       Hollywood, Maryland 20636 USA
%
%       Voice: (301) 373-9600
%       EMail: cups-info@cups.org
%         WWW: http://www.cups.org
%
/CENTER {			% Draw centered text
				% (name) CENTER -
  dup stringwidth pop		% Get the width of the string
  0.5 mul neg 0 rmoveto		% Shift left 1/2 of the distance
  show				% Show the string
} bind def
/RIGHT {			% Draw right-justified text
				% (name) RIGHT -
  dup stringwidth pop		% Get the width of the string
  neg 0 rmoveto			% Shift left the entire distance
  show				% Show the string
} bind def
/NUMBER {			% Draw a number
				% power n NUMBER -
  1 index 1 eq {		% power == 1?
    round cvi exch pop		% Convert "n" to integer
  } {
    1 index mul round exch div	% Truncate extra decimal places
  } ifelse
  100 string cvs show		% Convert to a string and show it...
} bind def
/CUPSLOGO {			% Draw the CUPS logo
				% height CUPSLOGO
  % Start with a big C...
  /Helvetica findfont 1 index scalefont setfont
  0 setgray
  0 0 moveto
  (C) show

  % Then "UNIX Printing System" much smaller...
  /Helvetica-Bold findfont 1 index 9 div scalefont setfont
  0.25 mul
  dup dup 2.0 mul moveto
  (UNIX) show
  dup dup 1.6 mul moveto
  (Printing) show
  dup 1.2 mul moveto
  (System) show
} bind def
/ESPLOGO {			% Draw the ESP logo
				% height ESPLOGO
  % Compute the size of the logo...
  0 0
  2 index 1.5 mul 3 index

  % Do the "metallic" fill from 10% black to 40% black...
  1 -0.001 0 {
    dup			% loopval
    -0.15 mul		% loopval * -0.15
    0.9 add		% 0.9 - loopval * 0.15
    setgray		% set gray shade

    0			% x
    1 index neg		% loopval
    1 add		% 1 - loopval
    3 index		% height
    mul			% height * (1 - loopval)
    moveto		% starting point

    dup			% loopval
    3 index		% width
    mul			% loopval * width
    2 index		% height
    lineto		% Next point

    0			% x
    2 index		% height
    lineto		% Next point

    closepath
    fill

    dup			% loopval
    0.15 mul		% loopval * 0.15
    0.6 add		% 0.6 + loopval * 0.15
    setgray

    dup			% loopval
    neg 1 add		% 1 - loopval
    3 index		% width
    mul			% (1 - loopval) * width
    0			% y
    moveto		% Starting point

    2 index		% width
    exch		% loopval
    2 index		% height
    mul			% loopval * height
    lineto		% Next point

    1 index		% width
    0			% y
    lineto		% Next point

    closepath
    fill
  } for

  0 setgray rectstroke

  /Helvetica-BoldOblique findfont 1 index 3 div scalefont setfont
  dup 40 div

  dup 4 mul 1 index 25 mul moveto (E) show
  dup 10 mul 1 index 15 mul moveto (S) show
  dup 16 mul 1 index 5 mul moveto (P) show

  /Helvetica-BoldOblique findfont 2 index 5 div scalefont setfont
  dup 14 mul 1 index 29 mul moveto (asy) show
  dup 20 mul 1 index 19 mul moveto (oftware) show
  dup 26 mul 1 index 9 mul moveto (roducts) show

  pop
} bind def
%%EndResource
%%EndProlog
%%BeginSetup
% Disable CTRL-D as an end-of-file marker...
userdict dup(\004)cvn{}put (\004\004)cvn{}put
[{
%%BeginFeature: *Duplex None
<</Duplex false>>setpagedevice
%%EndFeature
} stopped cleartomark
[{
%%BeginFeature: *InputSlot Default
%%EndFeature
} stopped cleartomark
[{
%%BeginFeature: *Option1 False
%%EndFeature
} stopped cleartomark
% x y w h ESPrc - Clip to a rectangle.
userdict/ESPrc/rectclip where{pop/rectclip load}
{{newpath 4 2 roll moveto 1 index 0 rlineto 0 exch rlineto
neg 0 rlineto closepath clip newpath}bind}ifelse put
% x y w h ESPrf - Fill a rectangle.
userdict/ESPrf/rectfill where{pop/rectfill load}
{{gsave newpath 4 2 roll moveto 1 index 0 rlineto 0 exch rlineto
neg 0 rlineto closepath fill grestore}bind}ifelse put
% x y w h ESPrs - Stroke a rectangle.
userdict/ESPrs/rectstroke where{pop/rectstroke load}
{{gsave newpath 4 2 roll moveto 1 index 0 rlineto 0 exch rlineto
neg 0 rlineto closepath stroke grestore}bind}ifelse put
userdict/ESPpa(SystemLow-SystemHigh)put
userdict/ESPlf /Nimbus-Mono findfont 12 scalefont put
userdict/ESPwl{
  ESPlf setfont
  ESPpa stringwidth pop dup 12 add exch -0.5 mul 306 add
 
  1 setgray
  dup 6 sub 10 3 index 22 ESPrf
  dup 6 sub 758 3 index 20 ESPrf
  0 setgray
  dup 6 sub 10 3 index 22 ESPrs
  dup 6 sub 758 3 index 20 ESPrs
  dup 18 moveto ESPpa show
  766 moveto ESPpa show
  pop
}bind put
%%EndSetup
%%Page: 1 1
%%PageBoundingBox: 0 0 612 792
%%BeginPageSetup
%%EndPageSetup
gsave

  % Determine the imageable area and device resolution...
  initclip newpath clippath pathbbox	% Get bounding rectangle
  72 div /pageTop exch def		% Get top margin in inches
  72 div /pageRight exch def		% Get right margin in inches
  72 div /pageBottom exch def		% Get bottom margin in inches
  72 div /pageLeft exch def		% Get left margin in inches

  /pageWidth pageRight pageLeft sub def	% pageWidth = pageRight - pageLeft
  /pageHeight pageTop pageBottom sub def% pageHeight = pageTop - pageBottom

  /boxWidth				% width of text box
  pageWidth pageHeight lt
  { pageWidth 54 mul }
  { pageHeight 42 mul }
  ifelse def

  newpath				% Clear bounding path

  % Create fonts...
  /bigFont /Helvetica-Bold findfont	% bigFont = Helvetica-Bold
  pageHeight 3 mul scalefont def	% size = pageHeight * 3 (nominally 33)

  /mediumFont /Helvetica findfont	% mediumFont = Helvetica
  pageHeight 1.5 mul scalefont def	% size = pageHeight * 1.5 (nominally 16.5)

  % Offset page to account for lower-left margin...
  pageLeft 72 mul
  pageBottom 72 mul
  translate

  % Job information box...
  pageWidth 36 mul 9 add		% x = pageWidth * 1/2 * 72 + 9
  boxWidth 0.5 mul sub			% x-= 1/2 box width
  pageHeight 30 mul 9 sub		% y = pageHeight * 1/2 * 72 - 9
  boxWidth				% w = box width
  pageHeight 14 mul			% h = pageHeight * 1/2 * 72
  0.5 setgray rectfill			% Draw a shadow

  pageWidth 36 mul			% x = pageWidth * 1/2 * 72
  boxWidth 0.5 mul sub			% x-= 1/2 box width
  pageHeight 30 mul			% y = pageHeight * 1/4 * 72
  boxWidth				% w = box width
  pageHeight 14 mul			% h = pageHeight * 1/2 * 72

  4 copy 1 setgray rectfill		% Clear the box to white
  0 setgray rectstroke			% Draw a black box around it...

  % Job information text...
  mediumFont setfont			% Medium sized font

  pageWidth 36 mul			% x = pageWidth * 1/2 * 72
  pageHeight 36 mul			% y = pageHeight * 1/2 * 72
  pageHeight 5 mul add			% y += 2 lines
  2 copy				% Copy X & Y
  moveto
  (Job ID: ) RIGHT
  moveto
  (tests-42) show

  pageWidth 36 mul			% x = pageWidth * 1/2 * 72
  pageHeight 36 mul			% y = pageHeight * 1/2 * 72
  pageHeight 2 mul add			% y += 1 line
  2 copy				% Copy X & Y
  moveto
  (Title: ) RIGHT
  moveto
  (hello-world.ps) show

  pageWidth 36 mul			% x = pageWidth * 1/2 * 72
  pageHeight 36 mul			% y = pageHeight * 1/2 * 72
  pageHeight -1 mul add			% y -= 1 line
  2 copy				% Copy X & Y
  moveto
  (Requesting User: ) RIGHT
  moveto
  (root) show

  pageWidth 36 mul			% x = pageWidth * 1/2 * 72
  pageHeight 36 mul			% y = pageHeight * 1/2 * 72
  pageHeight -4 mul add			% y -= 2 lines
  2 copy				% Copy X & Y
  moveto
  (Billing Info: ) RIGHT
  moveto
  () show

  % Then the CUPS logo....
  gsave
    pageWidth 4 mul
    pageWidth 6 mul
    translate
    pageWidth 9 mul CUPSLOGO
  grestore

  % And the ESP logo....
  gsave
    pageWidth 59 mul
    pageWidth 6 mul
    translate
    pageWidth 6 mul ESPLOGO
  grestore
% Show the page...
grestore
showpage
%
% End of "\$Id: mls_template,v 1.1 2005/06/27 18:44:46 colmo Exp $".
%
gsave
ESPwl
grestore
ESPshowpage
userdict/showpage/ESPshowpage load put

%%Trailer
%%Pages: 1
%%BoundingBox: 0 0 612 792
%%EOF
%!PS-Adobe-3.0
%%BoundingBox: 12 12 600 780
%%Pages: 1
%%For: (root)
%%Title: (hello-world.ps)
%RBINumCopies: 1
%%EndComments
%%BeginProlog
%%EndProlog
%%BeginSetup
% Disable CTRL-D as an end-of-file marker...
userdict dup(\004)cvn{}put (\004\004)cvn{}put
[{
%%BeginFeature: *Duplex None
<</Duplex false>>setpagedevice
%%EndFeature
} stopped cleartomark
[{
%%BeginFeature: *InputSlot Default
%%EndFeature
} stopped cleartomark
[{
%%BeginFeature: *Option1 False
%%EndFeature
} stopped cleartomark
% x y w h ESPrc - Clip to a rectangle.
userdict/ESPrc/rectclip where{pop/rectclip load}
{{newpath 4 2 roll moveto 1 index 0 rlineto 0 exch rlineto
neg 0 rlineto closepath clip newpath}bind}ifelse put
% x y w h ESPrf - Fill a rectangle.
userdict/ESPrf/rectfill where{pop/rectfill load}
{{gsave newpath 4 2 roll moveto 1 index 0 rlineto 0 exch rlineto
neg 0 rlineto closepath fill grestore}bind}ifelse put
% x y w h ESPrs - Stroke a rectangle.
userdict/ESPrs/rectstroke where{pop/rectstroke load}
{{gsave newpath 4 2 roll moveto 1 index 0 rlineto 0 exch rlineto
neg 0 rlineto closepath stroke grestore}bind}ifelse put
userdict/ESPwl{}bind put
%%EndSetup
%%Page: 1 1
%%BeginPageSetup
%%EndPageSetup
%%BeginDocument: nondsc
%!
% Sample of printing text

/Times-Roman findfont	% Get the basic font
20 scalefont		% Scale the font to 20 points
setfont			% Make it the current font

newpath			% Start a new path
72 72 moveto		% Lower left corner of text at (72, 72)
(Hello, world!) show    % Typeset "Hello, world!"

showpage
%%EndDocument
%%EOF
%!PS-Adobe-3.0
%%LanguageLevel: 1
%%DocumentData: Clean7Bit
%%DocumentSuppliedResources: procset bannerprint/1.0
%%DocumentNeededResources: font Helvetica Helvetica-Bold Times-Roman
%%Creator: Michael Sweet, Easy Software Products
%%CreationDate: May 10, 2000
%%Title: Test Page
%%For: (root)
%RBINumCopies: 1
%%Pages: (atend)
%%BoundingBox: (atend)
%%EndComments
%%BeginProlog
userdict/ESPshowpage/showpage load put
userdict/showpage{}put
%%BeginResource procset bannerprint 1.1 0
%
%   PostScript banner page for the Common UNIX Printing System ("CUPS").
%
%   Copyright 1993-2005 by Easy Software Products
%
%   These coded instructions, statements, and computer programs are the
%   property of Easy Software Products and are protected by Federal
%   copyright law.  Distribution and use rights are outlined in the file
%   "LICENSE.txt" which should have been included with this file.  If this
%   file is missing or damaged please contact Easy Software Products
%   at:
%
%       Attn: CUPS Licensing Information
%       Easy Software Products
%       44141 Airport View Drive, Suite 204
%       Hollywood, Maryland 20636 USA
%
%       Voice: (301) 373-9600
%       EMail: cups-info@cups.org
%         WWW: http://www.cups.org
%
/CENTER {			% Draw centered text
				% (name) CENTER -
  dup stringwidth pop		% Get the width of the string
  0.5 mul neg 0 rmoveto		% Shift left 1/2 of the distance
  show				% Show the string
} bind def
/RIGHT {			% Draw right-justified text
				% (name) RIGHT -
  dup stringwidth pop		% Get the width of the string
  neg 0 rmoveto			% Shift left the entire distance
  show				% Show the string
} bind def
/NUMBER {			% Draw a number
				% power n NUMBER -
  1 index 1 eq {		% power == 1?
    round cvi exch pop		% Convert "n" to integer
  } {
    1 index mul round exch div	% Truncate extra decimal places
  } ifelse
  100 string cvs show		% Convert to a string and show it...
} bind def
/CUPSLOGO {			% Draw the CUPS logo
				% height CUPSLOGO
  % Start with a big C...
  /Helvetica findfont 1 index scalefont setfont
  0 setgray
  0 0 moveto
  (C) show

  % Then "UNIX Printing System" much smaller...
  /Helvetica-Bold findfont 1 index 9 div scalefont setfont
  0.25 mul
  dup dup 2.0 mul moveto
  (UNIX) show
  dup dup 1.6 mul moveto
  (Printing) show
  dup 1.2 mul moveto
  (System) show
} bind def
/ESPLOGO {			% Draw the ESP logo
				% height ESPLOGO
  % Compute the size of the logo...
  0 0
  2 index 1.5 mul 3 index

  % Do the "metallic" fill from 10% black to 40% black...
  1 -0.001 0 {
    dup			% loopval
    -0.15 mul		% loopval * -0.15
    0.9 add		% 0.9 - loopval * 0.15
    setgray		% set gray shade

    0			% x
    1 index neg		% loopval
    1 add		% 1 - loopval
    3 index		% height
    mul			% height * (1 - loopval)
    moveto		% starting point

    dup			% loopval
    3 index		% width
    mul			% loopval * width
    2 index		% height
    lineto		% Next point

    0			% x
    2 index		% height
    lineto		% Next point

    closepath
    fill

    dup			% loopval
    0.15 mul		% loopval * 0.15
    0.6 add		% 0.6 + loopval * 0.15
    setgray

    dup			% loopval
    neg 1 add		% 1 - loopval
    3 index		% width
    mul			% (1 - loopval) * width
    0			% y
    moveto		% Starting point

    2 index		% width
    exch		% loopval
    2 index		% height
    mul			% loopval * height
    lineto		% Next point

    1 index		% width
    0			% y
    lineto		% Next point

    closepath
    fill
  } for

  0 setgray rectstroke

  /Helvetica-BoldOblique findfont 1 index 3 div scalefont setfont
  dup 40 div

  dup 4 mul 1 index 25 mul moveto (E) show
  dup 10 mul 1 index 15 mul moveto (S) show
  dup 16 mul 1 index 5 mul moveto (P) show

  /Helvetica-BoldOblique findfont 2 index 5 div scalefont setfont
  dup 14 mul 1 index 29 mul moveto (asy) show
  dup 20 mul 1 index 19 mul moveto (oftware) show
  dup 26 mul 1 index 9 mul moveto (roducts) show

  pop
} bind def
%%EndResource
%%EndProlog
%%BeginSetup
% Disable CTRL-D as an end-of-file marker...
userdict dup(\004)cvn{}put (\004\004)cvn{}put
[{
%%BeginFeature: *Duplex None
<</Duplex false>>setpagedevice
%%EndFeature
} stopped cleartomark
[{
%%BeginFeature: *InputSlot Default
%%EndFeature
} stopped cleartomark
[{
%%BeginFeature: *Option1 False
%%EndFeature
} stopped cleartomark
% x y w h ESPrc - Clip to a rectangle.
userdict/ESPrc/rectclip where{pop/rectclip load}
{{newpath 4 2 roll moveto 1 index 0 rlineto 0 exch rlineto
neg 0 rlineto closepath clip newpath}bind}ifelse put
% x y w h ESPrf - Fill a rectangle.
userdict/ESPrf/rectfill where{pop/rectfill load}
{{gsave newpath 4 2 roll moveto 1 index 0 rlineto 0 exch rlineto
neg 0 rlineto closepath fill grestore}bind}ifelse put
% x y w h ESPrs - Stroke a rectangle.
userdict/ESPrs/rectstroke where{pop/rectstroke load}
{{gsave newpath 4 2 roll moveto 1 index 0 rlineto 0 exch rlineto
neg 0 rlineto closepath stroke grestore}bind}ifelse put
userdict/ESPpa(SystemLow-SystemHigh)put
userdict/ESPlf /Nimbus-Mono findfont 12 scalefont put
userdict/ESPwl{
  ESPlf setfont
  ESPpa stringwidth pop dup 12 add exch -0.5 mul 306 add
 
  1 setgray
  dup 6 sub 10 3 index 22 ESPrf
  dup 6 sub 758 3 index 20 ESPrf
  0 setgray
  dup 6 sub 10 3 index 22 ESPrs
  dup 6 sub 758 3 index 20 ESPrs
  dup 18 moveto ESPpa show
  766 moveto ESPpa show
  pop
}bind put
%%EndSetup
%%Page: 1 1
%%PageBoundingBox: 0 0 612 792
%%BeginPageSetup
%%EndPageSetup
gsave

  % Determine the imageable area and device resolution...
  initclip newpath clippath pathbbox	% Get bounding rectangle
  72 div /pageTop exch def		% Get top margin in inches
  72 div /pageRight exch def		% Get right margin in inches
  72 div /pageBottom exch def		% Get bottom margin in inches
  72 div /pageLeft exch def		% Get left margin in inches

  /pageWidth pageRight pageLeft sub def	% pageWidth = pageRight - pageLeft
  /pageHeight pageTop pageBottom sub def% pageHeight = pageTop - pageBottom

  /boxWidth				% width of text box
  pageWidth pageHeight lt
  { pageWidth 54 mul }
  { pageHeight 42 mul }
  ifelse def

  newpath				% Clear bounding path

  % Create fonts...
  /bigFont /Helvetica-Bold findfont	% bigFont = Helvetica-Bold
  pageHeight 3 mul scalefont def	% size = pageHeight * 3 (nominally 33)

  /mediumFont /Helvetica findfont	% mediumFont = Helvetica
  pageHeight 1.5 mul scalefont def	% size = pageHeight * 1.5 (nominally 16.5)

  % Offset page to account for lower-left margin...
  pageLeft 72 mul
  pageBottom 72 mul
  translate

  % Job information box...
  pageWidth 36 mul 9 add		% x = pageWidth * 1/2 * 72 + 9
  boxWidth 0.5 mul sub			% x-= 1/2 box width
  pageHeight 30 mul 9 sub		% y = pageHeight * 1/2 * 72 - 9
  boxWidth				% w = box width
  pageHeight 14 mul			% h = pageHeight * 1/2 * 72
  0.5 setgray rectfill			% Draw a shadow

  pageWidth 36 mul			% x = pageWidth * 1/2 * 72
  boxWidth 0.5 mul sub			% x-= 1/2 box width
  pageHeight 30 mul			% y = pageHeight * 1/4 * 72
  boxWidth				% w = box width
  pageHeight 14 mul			% h = pageHeight * 1/2 * 72

  4 copy 1 setgray rectfill		% Clear the box to white
  0 setgray rectstroke			% Draw a black box around it...

  % Job information text...
  mediumFont setfont			% Medium sized font

  pageWidth 36 mul			% x = pageWidth * 1/2 * 72
  pageHeight 36 mul			% y = pageHeight * 1/2 * 72
  pageHeight 5 mul add			% y += 2 lines
  2 copy				% Copy X & Y
  moveto
  (Job ID: ) RIGHT
  moveto
  (tests-42) show

  pageWidth 36 mul			% x = pageWidth * 1/2 * 72
  pageHeight 36 mul			% y = pageHeight * 1/2 * 72
  pageHeight 2 mul add			% y += 1 line
  2 copy				% Copy X & Y
  moveto
  (Title: ) RIGHT
  moveto
  (hello-world.ps) show

  pageWidth 36 mul			% x = pageWidth * 1/2 * 72
  pageHeight 36 mul			% y = pageHeight * 1/2 * 72
  pageHeight -1 mul add			% y -= 1 line
  2 copy				% Copy X & Y
  moveto
  (Requesting User: ) RIGHT
  moveto
  (root) show

  pageWidth 36 mul			% x = pageWidth * 1/2 * 72
  pageHeight 36 mul			% y = pageHeight * 1/2 * 72
  pageHeight -4 mul add			% y -= 2 lines
  2 copy				% Copy X & Y
  moveto
  (Billing Info: ) RIGHT
  moveto
  () show

  % Then the CUPS logo....
  gsave
    pageWidth 4 mul
    pageWidth 6 mul
    translate
    pageWidth 9 mul CUPSLOGO
  grestore

  % And the ESP logo....
  gsave
    pageWidth 59 mul
    pageWidth 6 mul
    translate
    pageWidth 6 mul ESPLOGO
  grestore
% Show the page...
grestore
showpage
%
% End of "\$Id: mls_template,v 1.1 2005/06/27 18:44:46 colmo Exp $".
%
gsave
ESPwl
grestore
ESPshowpage
userdict/showpage/ESPshowpage load put

%%Trailer
%%Pages: 1
%%BoundingBox: 0 0 612 792
%%EOF
EOF
# add an EOF for good measure, but no linefeed
echo -n  >> $LABELED

EXECCON=staff_u:lspp_test_r:lspp_harness_t:SystemLow-SystemHigh
FILECON=staff_u:object_r:sysadm_home_t:SystemLow
JOBNO=42
OUTFILE=hello-output.ps
PRINTER=tests

setup_cupsd
create_socket_printer $PRINTER

chcon $FILECON $INFILE
prepend_cleanup delete_printer $PRINTER
prepend_cleanup rm $INFILE $OUTFILE $LABELED

# test
runcon $EXECCON /usr/bin/lpr -P $PRINTER -o job-id=$JOBNO $INFILE
create_socket_listener $OUTFILE

# verify
msg_1="job=.* auid=$(</proc/self/loginuid) acct=root printer=$PRINTER title=$INFILE obj=$EXECCON label=SystemLow-SystemHigh"

augrok -q type=USER_LABELED_EXPORT msg_1=~"$msg_1" || exit_fail "missing: \"$msg_1\""

if [ \! -f $OUTFILE ]; then
    exit_fail "File not found: $OUTFILE"
else
    sed -i \
	-e s/%%CreationDate.*$// \
	-e s/%%Creator.*$// \
	-e s/%%For.*$// \
	-e s/%%Title.*$// \
	$OUTFILE

    sed -i \
	-e s/%%CreationDate.*$// \
	-e s/%%Creator.*$// \
	-e s/%%For.*$// \
	-e s/%%Title.*$// \
	$LABELED

    diff $OUTFILE $LABELED || exit_fail "Labeled output does not match"
fi

# cleanup 

exit_pass

