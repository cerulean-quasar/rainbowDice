# Rainbow Dice
Rainbow Dice is a dice rolling program aimed at supporting any game requiring
dice including roll playing games.  Gamers can specify the number of dice, the
number of sides, what number each die starts at, on what if any number the dice
are re-rolled on, and whether two consecutive dice definitions should have their
results added or subtracted.  But you can choose the increment to be something
other than 1.  The result is printed at the top of the screen.

The dice are displayed using 3D (using the vulkan library or OpenGL 2.0 if Vulkan
is not available).  The program uses the accelerometer to calculate how fast the
dice are accelerating towards the walls of the virtual box they are in.  The dice
bounce to the top of the screen for added visability.  They bounce off the walls
and gain spin.  They bounce off each other as well.  The user can shake the device
to cause the dice to bounce around more.  The randomness comes from two sources:
the accelerometer (used in the way previously stated), and /dev/urandom.  The
/dev/urandom device is used to generate a random face to start facing the screen
and a random angle the dice is rotated about the z-axis (pointing out of the screen).

The user can select from several different themes to be used for all the screens.  In the
screen shots that follow, a different theme is displayed.

The following screen shows the app after rolling the dice.  Note that the
result is displayed and computed for the user in a separate text box.

<img src=screenshots/rainbowDice1.png width=200> <img src=screenshots/rainbowDice2.png width=200>

This picture shows the favorite selection and dice configuration screens.  Note
that gamers can enter several rows and an operation indicating whether the dice
in the two rows should be added or subtracted.

<img src=screenshots/rainbowDice_favoriteSelection.png width=200> <img src=screenshots/rainbowDice_customization1.png width=200> <img src=screenshots/rainbowDice_customization2.png width=400>

The roll results are also stored in a log file.  Ten results are kept at a time.

<img src=screenshots/rainbowDice_log.png width=200>

Please read the [design docs](https://github.com/cerulean-quasar/rainbowDice/blob/master/docs/design.md "design docs")
for more information on the design of Rainbow Dice.
