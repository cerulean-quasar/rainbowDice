# Rainbow Dice: dice roller
Rainbow Dice is a dice roller aimed at supporting any game requiring dice
including role playing games.  When using this dice roller, gamers can specify
the number of dice, the number of sides, what number each die starts at, on
what if any number the dice are re-rolled on, and whether two consecutive dice
definitions should have their results added or subtracted.  The dice roller
also allows the increment to be something other than 1.  The result of each
roll is printed at the top of the screen and is stored in a log file.  The log
file holds a max of 10 entries.

The dice roller displays the dice in 3D (using the Vulkan library or OpenGL 2.0
if Vulkan is not available).  The program uses the accelerometer to calculate
how fast the dice are accelerating towards the walls of the virtual box they
are in.  The dice fall toward the screen for added visability.  They
bounce off the walls and gain spin.  They bounce off each other as well.  The
user can shake the device to cause the dice to bounce around more.  The
randomness for Rainbow Dice: dice roller comes from two sources: the
accelerometer (used in the way previously stated), and /dev/urandom.  The
/dev/urandom device is used to generate a random face to be towards the screen
when the roll starts and a random angle the dice is rotated about the z-axis
(pointing out of the screen).  The /dev/urandom device is also used to give
the dice a random speed and position in x and y when the dice start rolling.

The dice roller has several different themes or skins that can be selected.

The following screen shows the app during and after rolling the dice.  Note that the
result is computed and displayed for the user in a separate text box.  Also note that
when the dice are done rolling, they move to the top of the screen in the same order
as displayed in the text box showing the result.

<img src=screenshots/rainbowDice1.png width=200> <img src=screenshots/rainbowDice2.png width=200>

This picture shows the order selection screen.  The user enters this activity by
clicking on "Choose Dice" from the main screen.  In this activity, the user can order
their dice configurations.  Changing the order in this list changes the order in the
list in the main activity at the top of the screen.  Users may wish to order their
dice based off of how often they use them or group them by game.

<img src=screenshots/rainbowDice_favoriteSelection.png width=200> 

See below for a picture of the dice customization screen.  The user enters this
activity when they select "New" from the order selection screen.  In this screen,
the user can configure a new dice customization.  Gamers can select the number of sides
the dice have, the start value (the lowest value to be displayed on the die), the next
value (the next lowest value to be displayed on the die), on what number the die should
be re-rolled on (empty indicates no re-rolling will be done).  Re-roll results will be
added (or subtracted if the operation is subtraction for that configuration) to the
original result.

<img src=screenshots/rainbowDice_customization1.png width=200> <img src=screenshots/rainbowDice_customization2.png width=400>

An example log file:

<img src=screenshots/rainbowDice_log.png width=200>

Please read the [design docs](https://github.com/cerulean-quasar/rainbowDice/blob/master/docs/design.md "design docs")
for more information on the design of Rainbow Dice: dice roller.  This includes
a discription of how random data is obtained and used.
