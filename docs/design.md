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
each die side have 5 triangles.  The two center triangles are used to hold the texture.

The following picture shows a side of the die divided as I specified.  One needs to find the
coordinates for vectors: p1prime, p1, p3 and p2.  The coordinates for p0, q, and r are known,
they are simply the vertices of the octohedron (or other similar shape).  The ratio: a/b is also
known and is just the ratio of the width to the height of the image containing the symbol to
be used as a texture on the die side.

<img src=/docs/pictures/dieSideFigure1.png>

Here, H and S are easily computed.  S is just the size of vector r - vector q.  That is:

`S = | r - q |`

H is the size of the vector that results from taking average of vectors q and r and
subtracting p0.  That is:

`H = | (r + q)/2 - p0 |`

Now, the triangle marked by p0, p1prime, and p1 is similar to the triangle marked by
p0, q, and r.  (Similar means that the angles are the same and but the sizes may be different.)
That is:

<img src=/docs/pictures/dieSideFigure2.jpeg>

The two triangles being similar implies that: `a/s = (H-b)/H` thus:

```
a/s + b/H = 1

(a/(bs) + 1/H) * b = 1

b = 1/(a/(bs) + 1/H) = sH/(Ha/b + s)

b/H = 1/((aH)/(bs) + 1)
```

Now, because it shows up many times, I will define the new variale:

```
k = a/b * H/s
```

Note that k is completely known since I already stated that a/b is known and specified
how to compute H and s.

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

Now I can compute the vector `p2`.  I can get a vector that points to the center of `s`
by taking the average of `r` and `q`.  Then we just need to add the bit between the center
of s ant `p2`.  The size of this bit of vector is `a/2` (since `(r+q)/2` bisects a as well).
the direction is `r-q` divided by its size or `(r-q)/s`.  That is:

```
p2 = (r+q)/2 + (r-q)a/(2s) = (1/2 + a/(2s))r + (1/2 - a/(2s))q

p2 = (1/2 + (1/2)(1/(1+1/k))) r + (1/2 - (1/2)(1/(1+1/k)))q

p2 = (1/2) ((2k+1)/(k+1)) r + (1/(2k+2))q

p2 = (1-1/(2k+2)) r + (1/(2k+2))q
```
