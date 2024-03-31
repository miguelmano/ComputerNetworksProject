# 1. start
* success
    * [x] start 096860
    * [x] sg 096860
* insuccess
    * [x] start
    * [x] start 12345
    * [x] start 096860; start 096860

# 2. play
* start 096860; rev
* success
    * OK: one letter
        * [x] play \<letter\>
        * [x] pl   \<letter\>
    * WIN: finish word one letter at a time, then start new game to check if game ended
        * [x] play \<letter\> and repeat; start 096860;
* insuccess
    * DUP
        * [x] play \<letter\>; play \<same letter\>
    * NOK
        * [x] play \<wrong letter\>
    * OVR
        * [x] play \<wrong letter\> (max_errors times); start 096860
    * INV
        * [x] if doesn't happen during previous during it's fine
    * ERR
        * [x] play \<number\>

# 3. guess
* start 096860; rev
* success
    * WIN: one word
        * [x] guess \<word\>; start 096860
        * [x] gw \<word\>; start 096860
* insuccess
    * [x] NOK: wrong word guess
    * [x] DUP: guess same word
    * [x] INV: if doesn't happen during previous during it's fine
    * [x] ERR: if doesn't happen during previous during it's fine
    * misc.
        * [x] no ongoing game
        * [x] guess word of different length or that doesn't contain actual letters
        
# 4. quit & exti
## quit
* start 096860;
* success (OK)
    * [x] quit
* insuccess
    * NOK
        * [x] quit (whitout starting a game)
    * ERR
        * [x] if doesn't happen during previous during it's fine
## exit
* [x] exit without a game started
* [x] exit with a game started

# misc.
* valid command
    * [x] <wrong command>