# Rainbow Dice

This program has some very mathmatical parts.  I would have loved to comment the code in these
areas however, comments would not work since these code segments require pictures to explain.
So, instead of comments, I have this design document.  I used the same variables in it as in
the code.

## How to the Textures Were Computed

The textures for this program are simply the numbers (or perhaps in future, other symbols) that
go on the sides of the die.  I could not use the normal method of using textures because the user
might choose any number to go on the die sides.  These textures had to be computed at run time.
So, I have a canvas that I draw the text into, the obtain the bitmap from it.  Then it is a matter
of fitting the bitmap on the die.

Fitting the bitmap on the six sided die is easy since each die side is a square.  I just use
each corner as a UV texture coordinate and don't worry to much about the stretching that might
occur because the text texture might not be a square but rather a rectangle.  However fitting
the bitmap on the other sided die (8 sided say), is more of a challenge.  On these die, each
side is a triangle.  So, one needs to know how to fit a triangle into a rectangle.  This makes
each die side have 5 triangles.  The two center triangles are used to hold the texture.  The
following picture shows a side of the die divided as I specified.  One needs to find the
coordinates for vectors: p1prime, p1, p3 and p2.  The coordinates for p0, q, and r are known,
they are simply the vertices of the octohedron (or other similar shape).
