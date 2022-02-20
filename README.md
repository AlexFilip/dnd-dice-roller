# D&D Dice Roller
A command line tool for rolling dice in Dungeons & Dragons. Simply state which dice you want to roll, ex. `d12`, `d20` or even one that doesn't exist `d97`. The program also accepts the number of times to roll the same die in a row, ex. `3d12`, `4d20`.
To exit, type `quit` or `exit`.


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
build/dice
```

