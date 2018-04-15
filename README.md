# Rainbow Dice
Rainbow Dice is a dice rolling program aimed at supporting any game requiring dice including roll playing games.
Gamers can specify the number of dice, the number of sides, what number each die starts at and whether two consecutive
dice definitions should have their results added or subtracted.  Currently all dice have numeric values on their sides.
Specifying other symbols is not possible.  But you can choose the increment to be something other than 1.  Dice configurations
can be configured so that dice are rerolled it a specified number is rolled.  The result is printed at the top of the screen.

The dice are displayed using 3D (using the vulkan library).  The program uses the accelerometer to calculate how fast the dice are
accelerating towards the walls of the virtual box they are in.  The dice bounce to the top of the screen for added visability.
They bounce off the walls and gain spin.  They bounce off each other as well.  The randomness in the rolls comes completely from
the physics of how the dice bounce and spin.

The following screen shows the app after rolling the dice.  Note that the result is displayed and computed for the user in a
separate text box.

![Rainbow Dice 1](/screenshots/rainbowDice1.png)

This picture shows the dice configuration screen.  Note that gamers can enter several rows and an operation indicating whether
the dice in the two rows should be added or subtracted.

![Rainbow Dice 4](/screenshots/rainbowDice4.png)
