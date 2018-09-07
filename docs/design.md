# Rainbow Dice

This program has some very mathmatical parts.  We would have loved to comment
the code in these areas however, comments would not work since these code
segments require pictures to explain.  So, instead of comments, we have this
design document.  We used the same variables in it as in the code.

## The Use of Randomness in Rainbow Dice

The dice in Rainbow Dice bounce around inside a virtual box.  They gain spin
when bouncing off of walls.  They bounce off each other as well.  The user can
shake the device to cause the dice to bounce around more.  They slow down due
to a fake viscosity in their virtual environment.  Gravity causes them to fall
to the top of the screen where they eventually get stuck.  With the input the
user provides in shaking and tilting the device, one would guess that there is
plenty of randomness in the way the dice roll.

However, just to be safe, we added the use of /dev/urandom to change the face
initially pointing upwards and to select an inital angle that each die is
rotated around the z-axis.  The method by which this was done is described
below.  For source code, look at `Random::getUInt` and `Random::getFloat` in
[random.cpp](https://github.com/cerulean-quasar/rainbowDice/blob/master/app/src/main/cpp/random.cpp).

For which die face is facing the screen, we read some data the size of an
unsigned int into an unsigned int from /dev/urandom.  Then, we checked to see
if the number read was less than the maximum unsigned int divided by the number
of faces multiplied by the number of faces (using integer division). If it is
not less, then we will throw away the data, and try again.  This will ensure
that when we take the random number modulo the number of faces, we will get
even probabilities for each face to be chosen as the face facing the screen.

For which angle the die starts out rotated around the z-axis where the z-axis
points out of the screen, we did the following. First we read some data the
size of an unsigned int into an unsigned int.  Then we assign this data to a
float.  Next we divide by the maximum unsigned int and multiply by 2pi.  This
gives us a float between 0 and 2pi with even distribution.  We use this number
to give the die an initial rotation around the z-axis.

Note, in this process (for both, determining the face pointing towards the
screen and the angle the die is rotated about the z-axis), we determine these
values for each die individually.

## How the Texture Coordinates Were Computed

### Background

The textures for this program are simply the numbers (or perhaps in future,
other symbols) that go on the sides of the die.  We could not use the normal
method of using textures because the user might choose any number to go on the
die sides.  These textures had to be computed at run time.  So, we have a canvas
that we draw the text into, the obtain the bitmap from it.  Then it is a matter
of fitting the bitmap on the die.

Fitting the bitmap on the six sided die is easy since each die side is a square.
We just use each corner as a UV texture coordinate.  We don't worry to much about
the stretching that might occur because the text texture might not be a square
but rather a rectangle.  However fitting the bitmap on the other sided die (8
sided say), is more of a challenge.

### Texture Coordinates for the Triangular Faced Dice.

The icosahedron, tetrahedron, octohedron, ten sided, thirty sided (and any sided
die that is not supported by the icosahedron, tetrahedron, octohedron, cube, or
dodecahedron), has triangular faces.  For these dice, we draw one triangle and 
for each vertex, we compute the texture coordinate as if we were drawing a
rectangle inside the triangle to hold the texture.  This produces a texture
coordinate which is outside of the texture for the lower two vertices.  We are
using clamp to edge, so since the texture has no symbol close to the edge, it
is just background that gets replicated.  The textures for all the faces of all
the dice are grouped together in one long texture.  They are pasted from top to
bottom.  Thus the top vertex's texture coordinate is only outside the texture if
it is the first symbol added to the texture.  For this reason, we place blank space
padding between each texture.

In order to calculate the texture coordinates we need to fit a rectangle inside
the triangular die face.  The following picture shows a side of the die with a
rectangle fitted inside.  One needs to find the coordinates for vectors:
p1prime, p1, p3 and p2.  The coordinates for p0, q, and r are known, they are
simply the vertices of the die face.  The ratio: a/b is also known and is just
the ratio of the width to the height of the image containing the symbol to be
used as a texture on the die side.

<img src=/docs/pictures/dieSideFigure1.png>

Here, `H` and `S` are easily computed.  `S` is just the size of vector `r` minus
vector `q`.  That is:

`S = | r - q |`

`H` is the size of the vector that results from taking average of vectors `q`
and `r` and subtracting `p0`.  That is:

`H = | (r + q)/2 - p0 |`

Now, the triangle marked by `p0`, `p1prime`, and `p1` is similar to the triangle
marked by `p0`, `q`, and `r`.  (Similar means that the angles are the same and
but the sizes may be different.) That is:

<img src=/docs/pictures/dieSideFigure2.jpeg>

The two triangles being similar implies that: `a/s = (H-b)/H` thus:

```
a/s + b/H = 1

(a/(bs) + 1/H) * b = 1

b = 1/(a/(bs) + 1/H) = sH/(Ha/b + s)

b/H = 1/((aH)/(bs) + 1)
```

Now, because it shows up many times, we will define the new variale:

```
k = a/b * H/s
```

Note that `k` is completely known since we already stated that `a/b` is known
and specified how to compute `H` and `s`.

```
b/H = 1/(K+1)
```

Now we have computed `b` in terms of `a/b`, `H` and `s`.  Next lets compute `a`.

```
a/s + b/H = 1

a(1/s + b/(aH)) = 1

a = 1/(1/s + b/(aH))

a/s = 1/(1+bs/(aH)) = 1/(1+1/k)

```

#### Computing p2

Now we can compute the vector `p2`.  We can get a vector that points to the
center of `s` by taking the average of `r` and `q`.  Then we just need to add
the bit between the center of `s` and `p2`.  The size of this bit of vector is
`a/2` (since `(r+q)/2` bisects a as well).  the direction is `r-q` divided by
its size or `(r-q)/s`.  That is:

```
p2 = (r+q)/2 + (r-q)a/(2s) = (1/2 + a/(2s))r + (1/2 - a/(2s))q

p2 = (1/2 + (1/2)(1/(1+1/k))) r + (1/2 - (1/2)(1/(1+1/k)))q

p2 = (1/2) ((2k+1)/(k+1)) r + (1/(2k+2))q

p2 = (1-1/(2k+2)) r + (1/(2k+2))q
```

#### Computing p3

Computing vector `p3` is similar to computing vector `p2`.  We again compute the
average of vectors `r` and `s`.  But this time, we need to subtract off the
vector `a(r-q)/(2s)` to get from the point between `r` and `s` to `p3`.  That
is:

```
p3 = (r+q)/2 - (r-q)a/(2s)

p3 = r/(2k+2) + (1 - 1/(2k+2))*q
```

#### Computing p1

To compute vector `p1`, we start off at vector `p2` which is now known.  Then we
need a vector which points up towards `p1`.  This is obtained by taking the
average of `r` and `q` again (which gets us a vector pointing to the center of
the line `s`), and then subtracting off `p0`.  This gets us a vector pointing
from the center of `q` and `r` to `p0`.  This vector is the right direction but
not the right length.  So, next we normalize it by dividing it by its length:
`H`.  Then, multiply it by the length we want: `b`.  That is:

```
p1 = p2 - (((r+q)/2 -p0)/H)b = p2 - ((r+q)/2 - p0)(1/(k+1))
```

#### Computing p1prime

Computing `p1prime` is the same as computing `p1` except we start at `p3`.  That is:

```
p1prime = p3 - (((r+q)/2 -p0)/H)b = p3 - ((r+q)/2 - p0)(1/(k+1))
```

#### Conclusion

Now with vectors: `p1`, `p1prime`, `p2`, and `p3`, we can compute the texture
coordinates for `p0`, `q`, and `r`.  We want the texture coordinates to be:

```
p1prime ---> (0.0, (1.0/totalNumberImages)*(textureToUse))
p1      ---> (1.0, (1.0/totalNumberImages)*(textureToUse))
p2      ---> (1.0, (1.0/totalNumberImages)*(textureToUse+1.0))
p3      ---> (0.0, (1.0/totalNumberImages)*(textureToUse+1.0))
```

Note the v-coordinate is not simply 0 or 1 because all the textures for all the
dice faces are appended together vertically.

Then we need to interpolate the u and v coordinates for the texture at `p0`, `q`,
and `r` such that the fragment shader produces the coordinates for the points as
we gave above.

##### Texture Coordinates for p0

`p0` is in the center of the texture square, thus we want the u-coordinate to be
in the center of the square.  So we make the u-coordinate to be half of the length
of the vector starting at point `p3` and points to point `p2`.  That is:

```
u = 0.5 * | p2 - p3 |
```

For the v-coordinate, we want it to be at the top of the padding.  So we use the
following formula:

```
( textureToUse - | 0.5 *(r+q) - p0 | + | p1 - p2 | ) * (1.0 - paddingTotal/totalNumberOfImages) + heightPadding*textureToUse
```

Here textureToUse is an integer representing how far down in the list of images
the image for this particular texture is.  paddingTotal is the total amount of
the padding for all of the images added together.  totalNumberOfImages is the
total number of textures in the image.  And heightPadding is the height of the
padding for one image.

How did we arrive to this very long formula?  Remeber the v coordinate for points
`p1` and `p1prime` is `(1.0/totalNumberImages)*(textureToUse)`.  But we are not at
`p1` or `p1prime` we are at p0.  Thus we want to subtract off the vertical distance
from `p1` to `p0` from textureToUse.  The distance from `p1` to `p0` is:

```
| 0.5 *(r+q) - p0 | - | p1 - p2 |
```

Here `0.5*(r+q)` is the average of the `r` and `q` vectors, so it is a vector that
points to the middle of the bottom edge of the triangle.  Subtracting off p0 gives
us a vector that points from the top of the triangle pointing down towards the middle
of the bottom edge.  The length of `p1 - p2` gives us the length of the side of the
texture square.

Next we want to subtract the total amount of padding in the whole image (including
all textures) because normally this value would range from 0.0 to 1.0, but we want
to remove the padding from this range, thus subtracting it out.

Then we simply divide the whole thing by `totalNumberOfImages` because we don't
want the whole image we only want the part of it that will be used for the textue
we are on.

##### Texture Coordinates for q

We use similar math to what we did for `p0` in finding the u-v texture coordinates
for `q`.  To find the u texture coordinate, we need to subtract off the length of
the vector pointing from from `p3` to `q`.  But we need to divide this value by
the length of `p2-p3`.  This is because we want the u-coordinate to span from
0.0 to 1.0.  Thus you have to "normalize" the distance `|q-p3|` by dividing it by
the distance `|p2-p3|`.  So the u-coordinate is:

```
0.0 - |q-p3|/|p2-p3|
```

The v-coordinate is simply the formula for the v-coordinate given above but adjusted
for the fact that the total image used for the texture includes padding:

```
(textureToUse + 1) * (1.0 - paddingTotal) / totalNumberOfImages + heightPadding*textureToUse
```

##### Texture Coordinates for r

The u-coordinate for `r` is very similar to the one for `q`.  This time, we need to
add the "normalized" length of `|r-p2|` to 1.0 (the u-coordinate for the texture at
`p2`).  Note that `|r-p2| = |q-p3|`.  So the u-coordinate for the texture at `r` is:

```
1.0 + |r-p2|/|p2-p3|
```

The v-coordinate for `r` is the same as the v-coordinate for `q`.

### The Cube Model.

The cube is also completely rendered in software.  Below is a display of the
points added to the vertices array for the top and bottom faces (where y is the
"up" axis).

<img src=/docs/pictures/cube_model.png>

### The Dodecahedron Model

The dodecahedron is completely rendered in software.  Below is a display of a
face of the dodecahedron.  The vertices are labeled the same as they are in the
code.  The display also shows how the dodecahedron face is split up into
triangles.

<img src=/docs/pictures/dodecahedron_face.png>

The triangles described by (p1,p2,D) and (p1,D,C) are used to hold the texture
for the number on the die face.

## Which Side is Up After a Roll

When the dice are done rolling, we need to detect which side is up so that we
can report the result to the user in a text format (as opposed to them looking
at the dice on the screen).  We determine that the dice are done rolling when
the reach a certain minimum speed and are close to the screen.  (The dice fall
towards the screen.)  They might not necesarily have a face flat against the
screen.  So, we need to detect which face is closest to being towards the screen
and force that face to be flat against the screen.

So first lets figure out which face is closest to facing the screen.  To do
this, we need to find a vector perpendicular and pointing outwards from each
face.  Then find the angle between this vector and the z-axis unit vector.  The
face for which this angle is the smallest is the face pointing towards the
screen.  (Note: positive z points out of the screen.)  This method will work on
a six sided die or the other shaped die.

A vector that is perpendicular to the face of the die is just the cross product
of two vectors starting at the same point and pointing towards each of the two
nearest points to the starting point.  We can get these vectors by subtracting
the vector describing the starting point from the vector describing the ending
point.  That is:

<img src=/docs/pictures/dieSideFigure3.jpeg>

A vector perpendicular to the face is the cross product of `(p0-q)` and `(r-q)`:

```
(p0-q) X (r-q)
```

Now in order to get the angle just normalize the vector: `(p0-q) X (r-q)`
(divide it by its length) and take the dot product of that with the z-axis unit
vector.  Then take the arccosine of the result.  That is:

```
theta = acos(((p0-q) X (r-q)) * z-axis)
```

Here `*` denotes the dot product and theta is the angle between the z-axis and
the vector perpendicular to the face of the dice.  We want to calculate theta
for each face then find the smallest theta.  Then we need to rotate the dice so
that theta goes to 0 for that face.
