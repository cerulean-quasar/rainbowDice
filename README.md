# Rainbow Dice: dice roller
Rainbow Dice is a dice roller aimed at supporting any game requiring dice
including role playing games.  When using this dice roller, gamers can specify
the number of dice, the number of sides, the numbers, words or emojis that go on each side, on
what if any side the dice are re-rolled on, and whether two consecutive dice
definitions should have their results added or subtracted.  The dice roller
also allows the symbols that go on the faces to be mapped to numeric values.
This is useful, for example, in the fudge dice system, the dice faces have "+", blank,
and "minus" and have values: 1, 0, and -1 respectively.  The result of each
roll is printed at the top of the screen and is stored in a log file.  The log
file holds a maximum of 10 entries.

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
clicking on "Reorder Dice" from the main screen.  In this activity, the user can order
their dice configurations.  Changing the order in this list changes the order in the
list in the main activity at the top of the screen.  Users may wish to order their
dice based off of how often they use them or group them by game.  Gamers can also
use this screen to edit dice configurations or delete them.

<img src=screenshots/rainbowDice_favoriteSelection.png width=200> 

See below for a picture of the dice customization screen.  The user enters this
activity when they select "New" from the main screen or the order selection screen, or the
user selects a dice configuration to be edited in the order selection screen.  In this screen,
the user can configure a new dice customization.  Gamers can select the number of dice,
the number of sides the dice have, whether they are to be added or subtracted, and the color of
the dice.

<img src=screenshots/rainbowDice_customization1.png width=200> <img src=screenshots/rainbowDice_customization2.png width=400>

If the user clicks on the details button in the dice customization screen, then the
Dice Sides screen will be displayed.  On the dice sides screen, gamers can select the
symbols to go on the dice faces, the numeric values if any that they represent.  And
whether the die will be rerolled if a specified face lands up.  Re-roll results will be
added (or subtracted if the operation is subtraction for that configuration) to the
original result. If no value is selected for the dice sides, then the results will just
list how many of each symbol was rolled.

<img src=screenshots/rainbowDice_diceSides.png width=200>

An example log file:

<img src=screenshots/rainbowDice_log.png width=200>

Please read the [design docs](https://github.com/cerulean-quasar/rainbowDice/blob/master/docs/design.md "design docs")
for more information on the design of Rainbow Dice: dice roller.  This includes
a discription of how random data is obtained and used.
