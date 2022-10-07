/* GNU Chess 5.0 - cmd.c - command parser
   Copyright (c) 1999-2002 Free Software Foundation, Inc.

   GNU Chess is based on the two research programs 
   Cobalt by Chua Kong-Sian and Gazebo by Stuart Cracraft.

   GNU Chess is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Chess is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Chess; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Contact Info: 
     bug-gnu-chess@gnu.org
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "version.h"
#include "common.h"
#include "eval.h"

#if HAVE_LIBREADLINE
# if HAVE_READLINE_READLINE_H
#  include <readline/readline.h>
#  include <readline/history.h>
# else
extern char* readline(char *);
extern void add_history(char *);
# endif
#endif

#define prompt ':'

extern int stage, InChkDummy, terminal;

#ifdef HAVE_LIBREADLINE
static char *inputstr;
#else
static char inputstr[INPUT_SIZE];
#endif

static char cmd[INPUT_SIZE], file[INPUT_SIZE], 
            s[INPUT_SIZE], logfile[INPUT_SIZE], 
            gamefile[INPUT_SIZE], userinput[INPUT_SIZE];

char subcmd[INPUT_SIZE],setting[INPUT_SIZE],subsetting[INPUT_SIZE];

void InputCmd ()
/*************************************************************************
 *
 *  This is the main user command interface driver.
 *
 *************************************************************************/
{
   const char *color[2] = { "White", "Black" };
   int suffix;
   int i;
   leaf *ptr; 
   int ncmds;
   char *x,*trim;

   CLEAR (flags, THINK);
   memset(userinput,0,sizeof(userinput));
   memset(cmd,0,sizeof(cmd));
#ifndef HAVE_LIBREADLINE /* Why is this necessary anyway? */
   memset(inputstr,0,sizeof(inputstr));
#endif

#ifdef HAVE_LIBREADLINE
	 if (isatty(STDIN_FILENO)) {
	    sprintf(s,"%s (%d) %c ", color[board.side], (GameCnt+1)/2 + 1, prompt);
	    inputstr = readline(s);
	    if (inputstr == NULL) return;
	    if (*inputstr) {
	       add_history(inputstr);
	    }
	    if (strlen(inputstr) > INPUT_SIZE-1) {
	       printf("Warning: Input line truncated to %d characters.\n", INPUT_SIZE -1 );
	       inputstr[INPUT_SIZE-1] = '\000';
	    }
	 } else {
	    inputstr = malloc(INPUT_SIZE);
	    if (inputstr == NULL) {
	       perror("InputCmd");
	       exit(EXIT_FAILURE);
	    }
	    fgets(inputstr, INPUT_SIZE, stdin);
	    if (inputstr[0]) {
	       inputstr[strlen(inputstr)-1] = 0;
	    }
	 }
#else /* !HAVE_LIBREADLINE */
	if (!(flags & XBOARD)) {
	  printf ("%s (%d) %c ", color[board.side], (GameCnt+1)/2 + 1, prompt);
	  fflush(stdout);
        }
	fgets (inputstr, INPUT_SIZE, stdin) ;
#endif /* HAVE_LIBREADLINE */

	cmd[0] = '\n';
	strcpy(userinput,inputstr);
	sscanf (inputstr, "%s %[^\n]", cmd, inputstr);
	if (cmd[0] == '\n')
	  goto done;
	cmd[0] = subcmd[0] = setting[0] = subsetting[0] = '\0';
        ncmds = sscanf (userinput,"%s %s %s %[^\n]",
			cmd,subcmd,setting,subsetting);

   /* Put options after command back in inputstr - messy */
   sprintf(inputstr,"%s %s %s",subcmd,setting,subsetting);

   trim = inputstr + strlen(inputstr) - 1;
   while ( trim>=inputstr && *trim==' ')
                *trim--='\0';

   if (strcmp (cmd, "quit") == 0 || strcmp (cmd, "exit") == 0)
      SET (flags, QUIT);
   else if (strcmp (cmd, "help") == 0)
      ShowHelp (inputstr);
   else if (strcmp (cmd, "show") == 0)
      ShowCmd (inputstr);
   else if (strncmp (cmd, "book", 4) == 0) {
      if (strncmp(inputstr, "add",3) == 0) {
        sscanf (inputstr, "add %s", file);
        if (access(file,F_OK) < 0) {
	  printf("The syntax to add a new book is:\n\n\tbook add file.pgn\n");
        } else {
          BookPGNReadFromFile (file);
	}
      } else if (strncmp (inputstr, "on", 2) == 0 || strncmp (inputstr, "prefer", 6) == 0) {
	bookmode = BOOKPREFER;
	printf("book now on.\n");
      } else if (strncmp (inputstr, "off", 3) == 0) {
	bookmode = BOOKOFF;
	printf("book now off.\n");
      } else if (strncmp (inputstr, "best", 4) == 0) {
	bookmode = BOOKBEST;
	printf("book now best.\n");
      } else if (strncmp (inputstr, "worst", 5) == 0) {
	bookmode = BOOKWORST;
	printf("book now worst.\n");
      } else if (strncmp (inputstr, "random", 6) == 0) {
	bookmode = BOOKRAND;
	printf("book now random.\n");
      }
   } else if (strcmp (cmd, "test") == 0)
      TestCmd (inputstr);
   else if (strcmp (cmd, "version") == 0)
      ShowVersion ();
   else if (strcmp (cmd, "pgnsave") == 0)
           {     
		if ( strlen(inputstr) > 0 && strlen(inputstr) < INPUT_SIZE )
      		  PGNSaveToFile (inputstr,"");
		else
		  printf("Invalid filename.\n");
	   }
   else if (strcmp (cmd, "pgnload") == 0)
      PGNReadFromFile (inputstr);
   else if (strcmp (cmd, "manual") == 0)
      SET (flags, MANUAL);
   else if (strcmp (cmd, "debug") == 0)
   {
      SET (flags, DEBUGG);
      Debugmvl = 0;
      if (strcmp (inputstr, "debug") == 0)
      {
         while (strcmp (inputstr, s))
         {
            sscanf (inputstr, "%s %[^\n]", s, inputstr);
            ptr = ValidateMove (s);
            Debugmv[Debugmvl++] = ptr->move;
            MakeMove (board.side, &ptr->move);
         } 
         i = Debugmvl;
         while (i)
         {
            UnmakeMove (board.side, &Debugmv[--i]);
         } 
      }
   }
   else if (strcmp (cmd, "force") == 0)
	SET (flags, MANUAL);
   else if (strcmp (cmd, "white") == 0)
	;
   else if (strcmp (cmd, "black") == 0)
	;
   else if (strcmp (cmd, "hard") == 0)
	;
   else if (strcmp (cmd, "easy") == 0)
	;
   else if (strcmp (cmd, "list") == 0) {
	if (inputstr[0] == '?')
	{
	  printf("name    - list known players alphabetically\n");
	  printf("score   - list by GNU best result first \n");
	  printf("reverse - list by GNU worst result first\n");
	} else {
          sscanf (inputstr, "%s %[^\n]", cmd, inputstr);
          if (inputstr == '\000') DBListPlayer("rscore");
	  else DBListPlayer(inputstr);
	}
   }
   else if (strcmp (cmd, "post") == 0)
	SET (flags, POST);
   else if (strcmp (cmd, "nopost") == 0)
	CLEAR (flags, POST);
   else if (strcmp (cmd, "name") == 0) {
      strcpy(name, inputstr);
      x = name;
      while (*x != '\000') {
        if (*x == ' ') {
	  *x = '\000';
	  break;
	}
        x++;
      }
      suffix = 0;
      for (;;) {
	sprintf(logfile,"log.%03d",suffix);
 	sprintf(gamefile,"game.%03d",suffix);
	if (access(logfile,F_OK) < 0) {
	  ofp = fopen(logfile,"w");
	  break;
	} else 
	  suffix++;
      }
   }
   else if (strcmp (cmd, "result") == 0) {
     if (ofp != stdout) {  
	fprintf(ofp, "result: %s\n",inputstr);
	fclose(ofp); 
	ofp = stdout;
        printf("Save to %s\n",gamefile);
        PGNSaveToFile (gamefile, inputstr);
	DBUpdatePlayer (name, inputstr);
     }
   }	
   else if (strcmp (cmd, "rating") == 0) {
      sscanf(inputstr,"%d %d",&myrating,&opprating); 
      fprintf(ofp,"my rating = %d, opponent rating = %d\n",myrating,opprating); 
      /* Change randomness of book based on opponent rating. */
      /* Basically we play narrower book the higher the opponent */
      if (opprating >= 1700) bookfirstlast = 2;
      else if (opprating >= 1700) bookfirstlast = 2;
      else bookfirstlast = 2;
   }
   else if (strcmp (cmd, "activate") == 0) {
	CLEAR (flags, TIMEOUT);
	CLEAR (flags, ENDED);
   }
   else if (strcmp (cmd, "new") == 0) {
     InitVars ();
     NewPosition ();
     CLEAR (flags, MANUAL);
     CLEAR (flags, THINK);
     myrating = opprating = 0;
   }
   else if (strcmp (cmd, "time") == 0) {
     sscanf (inputstr, "%s %[^\n]", s, inputstr);
     TimeLimit[1^board.side] = atoi(s) / 100.0f ;
   }
   else if (strcmp (cmd, "otim") == 0)
	;
   else if (strcmp (cmd, "random") == 0)
	;
   else if (strcmp (cmd, "hash") == 0)
   {
      sscanf (inputstr, "%s %[^\n]", cmd, inputstr);
      if (strcmp (cmd, "off") == 0)
         CLEAR (flags, USEHASH);
      else if (strcmp (cmd, "on") == 0)
         SET (flags, USEHASH);
      printf ("Hashing %s\n", flags & USEHASH ? "on" : "off");
   }
   else if (strcmp (cmd, "hashsize") == 0)
   {
      if (inputstr[0] == 0) {
	 printf("Current HashSize is %u slots\n", HashSize);
      } else {
	 i = atoi (inputstr);
	 TTHashMask = 0;
	 while ((i >>= 1) > 0)
	 {
	    TTHashMask <<= 1;
	    TTHashMask |= 1;
	 }
	 HashSize = TTHashMask + 1;
	 printf ("Adjusting HashSize to %u slots\n", HashSize);
	 InitHashTable (); 
      }
   }
   else if (strcmp (cmd, "null") == 0)
   {
      sscanf (inputstr, "%s %[^\n]", cmd, inputstr);
      if (strcmp (cmd, "off") == 0)
         CLEAR (flags, USENULL);
      else if (strcmp (cmd, "on") == 0)
         SET (flags, USENULL);
      printf ("Null moves %s\n", flags & USENULL ? "on" : "off");
   }
   else if (strcmp (cmd, "xboard") == 0)
   {
      sscanf (inputstr, "%s %[^\n]", cmd, inputstr);
      if (strcmp (cmd, "off") == 0)
         CLEAR (flags, XBOARD);
      else if (strcmp (cmd, "on") == 0)
         SET (flags, XBOARD);
      else if (!(flags & XBOARD)) { /* set if unset and only xboard called */
	 SET (flags, XBOARD);	    /* like in xboard/winboard usage */
      }
   }
   else if (strcmp (cmd, "protover") == 0)
   {
      if (flags & XBOARD) {
	/* Note: change this if "analyze" or "draw" commands are added, etc. */
	printf("feature setboard=1 analyze=0 ping=1 draw=0"
	       " variants=\"normal\" myname=\"%s %s\" done=1\n",
	       PROGRAM, VERSION);
	fflush(stdout);
      }
   }
   else if (strcmp (cmd, "depth") == 0 || strcmp (cmd, "sd") == 0) {
      SearchDepth = atoi (inputstr);
      printf("Search to a depth of %d\n",SearchDepth);
   }
   else if (strcmp (cmd, "level") == 0)
   {
      SearchDepth = 0;
      sscanf (inputstr, "%d %f %d", &TCMove, &TCTime, &TCinc);
      if (TCMove == 0) {
	TCMove =  35 /* MIN((5*(GameCnt+1)/2)+1,60) */;
	printf("TCMove = %d\n",TCMove);
	suddendeath = 1;
      } else
	suddendeath = 0;
      if (TCTime == 0) {
         SET (flags, TIMECTL);
	 SearchTime = TCinc / 2.0f ;
         printf("Fischer increment of %d seconds\n",TCinc);
      }
      else
      {
         SET (flags, TIMECTL);
         MoveLimit[white] = MoveLimit[black] = TCMove - (GameCnt+1)/2;
         TimeLimit[white] = TimeLimit[black] = TCTime * 60;
	 if (!(flags & XBOARD)) {
	   printf ("Time Control: %d moves in %.2f secs\n", 
	          MoveLimit[white], TimeLimit[white]);
	   printf("Fischer increment of %d seconds\n",TCinc);
	 }
      }
   }
   else if (strcmp (cmd, "load") == 0 || strcmp (cmd, "epdload") == 0)
   {
      LoadEPD (inputstr);
      if (!ValidateBoard())
      {
	 SET (flags, ENDED);
         printf ("Board is wrong!\n");
      }
   }
   else if (strcmp (cmd, "save") == 0 || strcmp (cmd, "epdsave") == 0)
	{  
	   if ( strlen(inputstr) > 0 )
             SaveEPD (inputstr);
	   else
	     printf("Invalid filename.\n");
	}
   else if (strcmp (cmd, "epd") == 0)
   {
      ParseEPD (inputstr);
      NewPosition();
      ShowBoard();
      printf ("\n%s : Best move = %s\n", id, solution); 
   }
   else if (strcmp (cmd, "setboard") == 0)
   {
      /* setboard uses FEN, not EPD, but ParseEPD will accept FEN too */
      ParseEPD (inputstr);
      NewPosition();
   }
   else if (strcmp (cmd, "ping") == 0)
   {
      /* If ping is received when we are on move, we are supposed to 
	 reply only after moving.  In this version of GNU Chess, we
	 never read commands while we are on move, so we don't have to
	 worry about that here. */
      printf("pong %s\n", inputstr);
      fflush(stdout);
   }
   else if (strcmp (cmd, "switch") == 0)
   {
      board.side = 1^board.side;
      printf ("%s to move\n", board.side == white ? "White" : "Black");
   }
   else if (strcmp (cmd, "go") == 0)
   {
      SET (flags, THINK);
      CLEAR (flags, MANUAL);
      CLEAR (flags, TIMEOUT);
      CLEAR (flags, ENDED);
      computer = board.side;
   }
   else if (strcmp (cmd, "solve") == 0 || strcmp (cmd, "solveepd") == 0)
      Solve (inputstr);
   else if (strcmp (cmd,"remove") == 0) {
    if (GameCnt >= 0) {
       CLEAR (flags, ENDED);
       CLEAR (flags, TIMEOUT);
       UnmakeMove (board.side, &Game[GameCnt].move);
       if (GameCnt >= 0) {
         UnmakeMove (board.side, &Game[GameCnt].move);
         if (!(flags & XBOARD))
           ShowBoard ();
       }
       PGNSaveToFile ("game.log","");
    } else
       printf ("No moves to undo! \n");
   }
   else if (strcmp (cmd, "undo") == 0)
   {
      if (GameCnt >= 0)
         UnmakeMove (board.side, &Game[GameCnt].move);
      else
	 printf ("No moves to undo! \n");
      MoveLimit[board.side]++;
      TimeLimit[board.side] += Game[GameCnt+1].et;
      if (!(flags & XBOARD)) ShowBoard ();
   }
   else if (strcmp (cmd, "bk") == 0)
   {
	/* Print moves from Open Book for Xboard/WinBoard */
	/* Lines must start with " " and end with blank line */
	/* No need to test for xboard as it is generally useful */
	BookQuery(1);
	printf("\n"); /* Blank line */
        fflush(stdout);
   }
   /* List commands we don't implement to avoid illegal moving them */
   /* Play variant, we instruct interface in protover we play normal */
   else if (strcmp (cmd, "variant") == 0)
	;
   /* accepted and rejected refers to protocol requests */
   else if (strcmp (cmd, "accepted") == 0)
	;
   else if (strcmp (cmd, "rejected") == 0)
	;
   /* Set total time for move to be N seconds is "st N" */
   else if (strcmp (cmd, "st") == 0)
   {
	/* Approximately level 1 0 N */
	sscanf(inputstr,"%d",&TCinc);
	suddendeath = 0 ;
	/* Allow a little fussiness for failing low etc */
	SearchTime = TCinc * 0.90f ;
        CLEAR (flags, TIMECTL);
   }
   /* Ignore draw offers */
   else if (strcmp (cmd, "draw") == 0)
	;
   /* Predecessor to setboard */
   else if (strcmp (cmd, "edit") == 0)
   {
	if ( flags & XBOARD )
	{
	 printf("tellusererror command 'edit' not implemented\n");
	 fflush(stdout);
	}
   }
   /* Give a possible move for the player to play */
   else if (strcmp (cmd, "hint") == 0)
   {
     /* Find next move in PV, ignore if none available */
     /* Code belongs in search.c with ShowLine ? */

     int pvar;
     if ((flags & USEHASH))
     {
        if (TTGetPV(board.side,2,rootscore,&pvar))
        {
	  /* Find all moves for ambiguity checks */
	  /* Otherwise xboard complains at ambiguous hints */
	  GenMoves(2); 

          SANMove(pvar,2);
          printf("Hint: %s\n", SANmv);
          fflush(stdout);
	}
     }
   }
   /* Move now, not applicable */
   else if (strcmp (cmd, "?") == 0)
	;
   /* Enter analysis mode */
   else if (strcmp (cmd, "analyze") == 0)
   {
	printf("Error (unknown command): analyze\n");
	fflush(stdout);
   }
   /* Our opponent is a computer */
   else if (strcmp (cmd, "computer") == 0)
	;

   /* everything else must be a move */
   else
   {
      ptr = ValidateMove (cmd);
      if (ptr != NULL) 
      {
	 SANMove (ptr->move, 1);
	 MakeMove (board.side, &ptr->move);
	 strcpy (Game[GameCnt].SANmv, SANmv);
	 printf("%d. ",GameCnt/2+1);
	 printf("%s",cmd);
	 if (ofp != stdout) {
	   fprintf(ofp,"%d. ",GameCnt/2+1);
	   fprintf(ofp,"%s",cmd);
	 }
	 putchar('\n');
	 fflush(stdout);
  	 if (ofp != stdout) {
	   fputc('\n',ofp);
	   fflush(ofp);
         }
         if (!(flags & XBOARD)) ShowBoard (); 
	 SET (flags, THINK);
      }
      else {
	/*
	 * Must Output Illegal move to prevent Xboard accepting illegal
	 * en passant captures and other subtle mistakes
	 */
	printf("Illegal move: %s\n",cmd);
 	fflush(stdout);
      }
   }
  done:
#ifdef HAVE_LIBREADLINE
   free(inputstr);
#endif
   return;
}



void ShowCmd (char *subcmd)
/************************************************************************
 *
 *  The show command driver section.
 *
 ************************************************************************/
{
   char cmd[INPUT_SIZE];

   sscanf (subcmd, "%s %[^\n]", cmd, subcmd);
   if (strcmp (cmd, "board") == 0)
      ShowBoard ();
   else if (strcmp (cmd, "rating") == 0)
   {
      printf("My rating = %d\n",myrating);
      printf("Opponent rating = %d\n",opprating);
   } 
   else if (strcmp (cmd, "time") == 0)
      ShowTime ();
   else if (strcmp (cmd, "moves") == 0)
   {
      GenCnt = 0;
      TreePtr[2] = TreePtr[1];
      GenMoves (1);      
      ShowMoveList (1);
      printf ("No. of moves generated = %lu\n", GenCnt);
   }
   else if (strcmp (cmd, "escape") == 0)
   {
      GenCnt = 0;
      TreePtr[2] = TreePtr[1];
      GenCheckEscapes (1);      
      ShowMoveList (1);
      printf ("No. of moves generated = %lu\n", GenCnt);
   }
   else if (strcmp (cmd, "noncapture") == 0)
   {
      GenCnt = 0;
      TreePtr[2] = TreePtr[1];
      GenNonCaptures (1);      
      FilterIllegalMoves (1);
      ShowMoveList (1);
      printf ("No. of moves generated = %lu\n", GenCnt);
   }
   else if (strcmp (cmd, "capture") == 0)
   {
      GenCnt = 0;
      TreePtr[2] = TreePtr[1];
      GenCaptures (1);      
      FilterIllegalMoves (1);
      ShowMoveList (1);
      printf ("No. of moves generated = %lu\n", GenCnt);
   }
   else if (strcmp (cmd, "eval") == 0 || strcmp (cmd, "score") == 0)
   {
      int s, wp, bp, wk, bk;
      BitBoard *b;

      phase = PHASE;
      GenAtaks ();
      FindPins (&pinned);
      hunged[white] = EvalHung(white);
      hunged[black] = EvalHung(black);
      b = board.b[white];
      pieces[white] = b[knight] | b[bishop] | b[rook] | b[queen]; 
      b = board.b[black];
      pieces[black] = b[knight] | b[bishop] | b[rook] | b[queen]; 
      wp = ScoreP (white);
      bp = ScoreP (black);
      wk = ScoreK (white);
      bk = ScoreK (black);
      printf ("White:  Mat:%4d/%4d  P:%d  N:%d  B:%d  R:%d  Q:%d  K:%d  Dev:%d  h:%d x:%d\n",
	board.pmaterial[white], board.material[white], wp, ScoreN(white), 
        ScoreB(white), ScoreR(white), ScoreQ(white), wk, 
        ScoreDev(white), hunged[white], ExchCnt[white]);
      printf ("Black:  Mat:%4d/%4d  P:%d  N:%d  B:%d  R:%d  Q:%d  K:%d  Dev:%d  h:%d x:%d\n",
	board.pmaterial[black], board.material[black], bp, ScoreN(black), 
        ScoreB(black), ScoreR(black), ScoreQ(black), bk,
        ScoreDev(black), hunged[black], ExchCnt[black]);
      printf ("Phase: %d\t", PHASE);
      s = ( EvaluateDraw () ? DRAWSCORE : Evaluate (-INFINITY, INFINITY));
      printf ("score = %d\n", s);
      printf ("\n");
      return;
   }
   else if (strcmp (cmd, "game") == 0)
     ShowGame ();
   else if (strcmp (cmd, "pin") == 0)
   {
      BitBoard b;
      GenAtaks ();
      FindPins (&b);
      ShowBitBoard (&b);
   }
}

void TestCmd (char *subcmd)
/*************************************************************************
 *
 *  The test command driver section.
 *
 *************************************************************************/
{
   char cmd[INPUT_SIZE];

   sscanf (subcmd, "%s %[^\n]", cmd, subcmd);
   if (strcmp (cmd, "movelist") == 0)
      TestMoveList ();
   else if (strcmp (cmd, "capture") == 0)
      TestCaptureList ();
   else if (strcmp (cmd, "movegenspeed") == 0)
      TestMoveGenSpeed ();
   else if (strcmp (cmd, "capturespeed") == 0)
      TestCaptureGenSpeed ();
   else if (strcmp (cmd, "eval") == 0)
      TestEval ();
   else if (strcmp (cmd, "evalspeed") == 0)
      TestEvalSpeed ();
}

/*
 * This is more or less copied from the readme, and the
 * parser is not very clever, so the lines containing
 * command names should not be indented, the lines with
 * explanations following them should be indented. Do not
 * use tabs for indentation, only spaces. CAPITALS are
 * reserved for parameters in the command names. The
 * array must be terminated by two NULLs.
 */

static const char * const helpstr[] = {
   "^C",
   " Typically the interrupt key stops a search in progress,",
   " makes the move last considered best and returns to the",
   " command prompt",
   "quit",
   "exit",
   " These quit or exit the game.",
   "help",
   " Produces a help blurb corresponding to this list of commands.",
   "book",
   " add - compiles book.dat from book.pgn",
   " on - enables use of book",
   " off - disables use of book",
   " worst - play worst move from book",
   " best - play best move from book",
   " random - play any move from book",
   " prefer - default, same as 'book on'",
   "version",
   " prints out the version of this program",
   "pgnsave FILENAME",
   " saves the game so far to the file from memory",
   "pgnload FILENAME",
   " loads the game in the file into memory",
   "force",
   "manual",
   " Makes the program stop moving. You may now enter moves",
   " to reach some position in the future.",
   " ",
   "white",
   " Program plays white",
   "black",
   " Program plays black",
   "go",
   " Computer takes whichever side is on move and begins its",
   " thinking immediately",
   "post",
   " Arranges for verbose thinking output showing variation, score,",
   " time, depth, etc.",
   "nopost",
   " Turns off verbose thinking output",
   "name NAME",
   " Lets you input your name. Also writes the log.nnn and a",
   " corresponding game.nnn file. For details please see",
   " auxillary file format sections.",
   "result",
   " Mostly used by Internet Chess server.",
   "activate",
   " This command reactivates a game that has been terminated automatically",
   " due to checkmate or no more time on the clock. However, it does not",
   " alter those conditions. You would have to undo a move or two or",
   " add time to the clock with level or time in that case.",
   "rating COMPUTERRATING OPPONENTRATING",
   " Inputs the estimated rating for computer and for its opponent",
   "new",
   " Sets up new game (i.e. positions in original positions)",
   "time",
   " Inputs time left in game for computer in hundredths of a second.",
   " Mostly used by Internet Chess server.",
   "hash",
   " on - enables using the memory hash table to speed search",
   " off - disables the memory hash table",
   "hashsize N",
   " Sets the hash table to permit storage of N positions",
   "null",
   " on - enables using the null move heuristic to speed search",
   " off - disables using the null move heuristic",
   "xboard",
   " on - enables use of xboard/winboard",
   " off - disables use of xboard/winboard",
   "depth N",
   " Sets the program to look N ply (half-moves) deep for every",
   " search it performs. If there is a checkmate or other condition",
   " that does not allow that depth, then it will not be ",
   "level MOVES MINUTES INCREMENT",
   " Sets time control to be MOVES in MINUTES with each move giving",
   " an INCREMENT (in seconds, i.e. Fischer-style clock).",
   "load",
   "epdload",
   " Loads a position in EPD format from disk into memory.",
   "save",
   "epdsave",
   " Saves game position into EPD format from memory to disk.",
   "switch",
   " Switches side to move",
   "solve FILENAME",
   "solveepd FILENAME",
   " Solves the positions in FILENAME",
   "remove",
   " Backs up two moves in game history",
   "undo",
   " Backs up one move in game history",
   "show",
   " board - displays the current board",
   " time - displays the time settings",
   " moves - shows all moves using one call to routine",
   " escape - shows moves that escape from check using one call to routine",
   " noncapture - shows non-capture moves",
   " capture - shows capture moves",
   " eval [or score] - shows the evaluation per piece and overall",
   " game - shows moves in game history",
   " pin - shows pinned pieces",
   "test",
   " movelist - reads in an epd file and shows legal moves for its entries",
   " capture - reads in an epd file and shows legal captures for its entries",
   " movegenspeed - tests speed of move generator",
   " capturespeed - tests speed of capture move generator",
   " eval - reads in an epd file and shows evaluation for its entries",
   " evalspeed tests speed of the evaluator",
   "bk",
   " show moves from opening book.",
   NULL,
   NULL
};

void ShowHelp (const char * command)
/**************************************************************************
 *
 *  Display all the help commands.
 *
 **************************************************************************/
{
   const char * const *p;
   int count, len;

   if (strlen(command)>0) {
      for (p=helpstr, count=0; *p; p++) {
	 if  (strncmp(*p, inputstr, strlen(command)) == 0) {
	    puts(*p);
	    while (*++p && **p != ' ') /* Skip aliases */ ;
	    for (; *p && **p == ' '; p++) {
	       puts(*p);
	    }
	    return;
	 }
      }
      printf("Help for command %s not found\n\n", command);
   }
   printf("List of commands: (help COMMAND to get more help)\n");
   for (p=helpstr, count=0; *p; p++) {
      len = strcspn(*p, " ");
      if (len > 0) {
	 count += printf("%.*s  ", len, *p);
	 if (count > 60) {
	    count = 0;
	    puts("");
	 }
      }
   }
   puts("");
}

