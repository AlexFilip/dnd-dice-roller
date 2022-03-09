# D&D Dice Roller
A command line tool for rolling dice in Dungeons & Dragons. Simply state which dice you want to roll, ex. `d12`, `d20` or even one that doesn't exist `d97`. The program also accepts the number of times to roll the same die in a row, ex. `3d12`, `4d20`.

## Installation
Make sure you have clang++ and git installed on your system
```bash
$ which clang++
/usr/bin/clang++ # or wherever it is installed on your system
$ which git
/usr/bin/git
````


If you do, download the repository, run the `compile` script and the `dice` executable will be created in the `build` folder. From here you can move it anywhere it is convenient for you.

```bash
git clone https://github.com/AlexFilip/DnDDiceRoller.git
cd DnDDiceRoller
./compile
```

## Usage
Start up the program with 
```
build/dice
```

Now you can input the dice you want to roll on one line with the format `d[0-9]+`:
```
d20 d12
```

You can specify the number of sides with a leading number:
```
2d6
```

If only 1 die is rolled in a command only the result of that die will be displayed. If more than 1 die is rolled, the program with print the sum of all rolls, maximum and minimum.
```
> 3d10
3d10:
  Total: 10
  Max: 7
  Min: 1
```

To exit, use either `quit` or `exit`.

The way the program is built now is to execute the command immediately after it is read. This means that you may get errors at the end of your input if you use unsupported characters at the end. For example:
```
> 2d10 d40;
2d10:
  Total: 3
  Max: 2
  Min: 1

d40:
  10

Error: Unknown character ';'
```

This behavior may be changed in the future.

## TODO: Inventory and items

Two keywords (`add` and `remove`) are currently reserved for adding and removing to an inventory.
Strings between double quotes are also parsed but are not currently used for anything.

