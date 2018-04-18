# Rainbow Dice

This program has some very mathmatical parts.  We would have loved to comment the code in these
areas however, comments would not work since these code segments require pictures to explain.
So, instead of comments, we have this design document.  We used the same variables in it as in
the code.

## How to the Textures Were Computed

### Background

The textures for this program are simply the numbers (or perhaps in future, other symbols) that
go on the sides of the die.  We could not use the normal method of using textures because the user
might choose any number to go on the die sides.  These textures had to be computed at run time.
So, we have a canvas that we draw the text into, the obtain the bitmap from it.  Then it is a matter
of fitting the bitmap on the die.

Fitting the bitmap on the six sided die is easy since each die side is a square.  We just use
each corner as a UV texture coordinate and don't worry to much about the stretching that might
occur because the text texture might not be a square but rather a rectangle.  However fitting
the bitmap on the other sided die (8 sided say), is more of a challenge.  On these die, each
side is a triangle.  So, one needs to know how to fit a triangle into a rectangle.  This makes
each die side have 5 triangles.  The two center triangles are used to hold the texture.

The following picture shows a side of the die divided as we specified.  One needs to find the
coordinates for vectors: p1prime, p1, p3 and p2.  The coordinates for p0, q, and r are known,
they are simply the vertices of the octohedron (or other similar shape).  The ratio: a/b is also
known and is just the ratio of the width to the height of the image containing the symbol to
be used as a texture on the die side.

<img src=/docs/pictures/dieSideFigure1.png>

Here, `H` and `S` are easily computed.  `S` is just the size of vector `r` minus vector `q`.
That is:

`S = | r - q |`

`H` is the size of the vector that results from taking average of vectors `q` and `r` and
subtracting `p0`.  That is:

`H = | (r + q)/2 - p0 |`

Now, the triangle marked by `p0`, `p1prime`, and `p1` is similar to the triangle marked by
`p0`, `q`, and `r`.  (Similar means that the angles are the same and but the sizes may be different.)
That is:

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

Note that `k` is completely known since we already stated that `a/b` is known and specified
how to compute `H` and `s`.

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

### Computing p2

Now we can compute the vector `p2`.  We can get a vector that points to the center of `s`
by taking the average of `r` and `q`.  Then we just need to add the bit between the center
of s ant `p2`.  The size of this bit of vector is `a/2` (since `(r+q)/2` bisects a as well).
the direction is `r-q` divided by its size or `(r-q)/s`.  That is:

```
p2 = (r+q)/2 + (r-q)a/(2s) = (1/2 + a/(2s))r + (1/2 - a/(2s))q

p2 = (1/2 + (1/2)(1/(1+1/k))) r + (1/2 - (1/2)(1/(1+1/k)))q

p2 = (1/2) ((2k+1)/(k+1)) r + (1/(2k+2))q

p2 = (1-1/(2k+2)) r + (1/(2k+2))q
```

### Computing p3

Computing vector `p3` is similar to computing vector `p2`.  We again compute the average of vectors
`r` and `s`.  But this time, we need to subtract off the vector `a(r-q)/(2s)` to get from the point
between `r` and `s` to `p3`.  That is:

```
p3 = (r+q)/2 - (r-q)a/(2s)

p3 = r/(2k+2) + (1 - 1/(2k+2))*q
```

### Computing p1

To compute vector `p1`, we start off at vector `p2` which is now known.  Then we need a vector which
points up towards `p1`.  This is obtained by taking the average of `r` and `q` again (which gets
us a vector pointing to the center of the line `s`), and then subtracting off `p0`.  This gets us
a vector pointing from the center of `q` and `r` to `p0`.  This vector is the right direction
but not the right length.  So, next we normalize it by dividing it by its length: `H`.  Then,
multiply it by the length we want: `b`.  That is:

```
p1 = p2 - (((r+q)/2 -p0)/H)b = p2 - ((r+q)/2 - p0)(1/(k+1))
```

### Computing p1prime

Computing `p1prime` is the same as computing `p1` except we start at `p3`.  That is:

```
p1prime = p3 - (((r+q)/2 -p0)/H)b = p3 - ((r+q)/2 - p0)(1/(k+1))
```

### Conclusion

Now with vectors: `p1`, `p1prime`, `p2`, and `p3`, we can create all triangles for a side of the
octohedron (or similar shape).  The vectors: `p1`, `p1prime`, `p2`, and `p3` are used as UV
coordinates for the textures containing the symbols that go on each die face.

## Which Side is Up After a Roll

When the dice are done rolling, we need to detect which side is up so that we can report the result
to the user in a text format (as opposed to them looking at the dice on the screen).  We determine
that the dice are done rolling when the reach a certain minimum speed and are close to the screen.
(The dice fall towards the screen.)  They might not necesarily have a face flat against the screen.
So, we need to detect which face is closest to being towards the screen and force that face to be
flat against the screen.

So first lets figure out which face is closest to facing the screen.  To do this, we need to find
a vector perpendicular and pointing outwards from each face.  Then find the angle between this
vector and the z-axis unit vector.  The face for which this angle is the smallest is the face
pointing towards the screen.  (Note: positive z points out of the screen.)  This method will work
on a six sided die or the other shaped die.

A vector that is perpendicular to the face of the die is just the cross product of two vectors
starting at the same point and pointing towards each of the two nearest points to the starting
point.  We can get these vectors by subtracting the vector describing the starting point from
the vector describing the ending point.  That is:

<img src=/docs/pictures/dieSideFigure3.jpeg>

A vector perpendicular to the face is the cross product of `(p0-q)` and `(r-q)`:

```
(p0-q) X (r-q)
```

Now in order to get the angle just normalize the vector: `(p0-q) X (r-q)` (divide it by its length)
and take the dot product of that with the z-axis unit vector.  Then take the arccosine of the result.
That is:

```
theta = acos(((p0-q) X (r-q)) * z-axis)
```

Here `*` denotes the dot product and theta is the angle between the z-axis and the vector perpendicular
to the face of the dice.  We want to calculate theta for each face then find the smallest theta.
Then we need to rotate the dice so that theta goes to 0 for that face.
