/* GNU Chess 5.0 - ttable.c - transposition table code
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
/*
 *
 */

#include <stdio.h>
#include <string.h>
#include "common.h"

void TTPut (int side, int depth, int ply, int alpha, int beta, 
	    int score, int move)
/****************************************************************************
 *
 *  Uses a two-tier depth-based transposition table.  The criteria for 
 *  replacement is as follows.
 *  1.  If the position is better than the hash position, move the hash
 *      position to the 2nd tier and store the current position. 
 *      Minor change:  the move is only done if the 1st slot and the
 *      current position don't have the same hash key.
 *  2.  Otherwise, simply store the position into the second position.
 *  The & -2 is a trick to clear the last bit making the offset even. 
 *
 ****************************************************************************/
{
   HashSlot *t;

   depth = depth/DEPTH;
   t = HashTab[side] + ((HashKey & TTHashMask) & -2); 
   if (depth < t->depth && !MATESCORE(score))
      t++;
   else if (t->flag && t->key != KEY(HashKey))
      *(t+1) = *t;

   if (t->flag)
      CollHashCnt++;
   TotalPutHashCnt++;
   t->move = move;
   t->key = KEY(HashKey);
   t->depth = depth;
   if (t->depth == 0)
      t->flag = QUIESCENT;
   else if (score >= beta)
      t->flag = LOWERBOUND;         
   else if (score <= alpha)
      t->flag = UPPERBOUND;
   else  
      t->flag = EXACTSCORE;

   if (MATESCORE(score))
      t->score = score + ( score > 0 ? ply : -ply);
   else
      t->score = score;
}


int TTGet (int side, int depth, int ply, 
	     int alpha __attribute__ ((unused)),
	     int beta  __attribute__ ((unused)), 
	     int *score, int *move)
/*****************************************************************************
 *
 *  Probe the transposition table.  There are 2 entries to be looked at as
 *  we are using a 2-tier transposition table.
 *
 *****************************************************************************/
{
   HashSlot *t;
   KeyType Key;

   TotalGetHashCnt++;
   t = HashTab[side] + ((HashKey & TTHashMask) & -2);  
   Key = KEY(HashKey);
   if (Key != t->key && Key != (++t)->key)
      return (0);

   depth = depth/DEPTH;
   GoodGetHashCnt++;
   *move = t->move;
   *score = t->score;
   if (t->depth == 0)
      return (QUIESCENT);
   if (t->depth < depth && !MATESCORE (t->score))
      return (POORDRAFT);
   if (MATESCORE(*score))
      *score -= (*score > 0 ? ply : -ply);
   return (t->flag);
}


int TTGetPV (int side, int ply, int score, int *move)
/*****************************************************************************
 *
 *  Probe the transposition table.  There are 2 entries to be looked at as
 *  we are using a 2-tier transposition table.  This routine merely wants to
 *  get the PV from the hash, nothing else.
 *
 *****************************************************************************/
{
   HashSlot *t;
   KeyType Key;
   int s;

   t = HashTab[side] + ((HashKey & TTHashMask) & -2);  
   Key = KEY(HashKey);
   s = t->score;
   if (MATESCORE(s))
      s -= (s > 0 ? ply : -ply);
   if (Key == t->key && ((ply & 1 && score == s)||(!(ply & 1) && score == -s)))
   {
      *move = t->move;
      return (1);
   }
   t++;
   s = t->score;
   if (MATESCORE(s))
      s -= (s > 0 ? ply : -ply);
   if (Key == t->key && ((ply & 1 && score == s)||(!(ply & 1) && score == -s)))
   {
      *move = t->move;
      return (1);
   }
   return (0); 
}


void TTClear ()
/****************************************************************************
 *   
 *  Zero out the transposition table.
 *
 ****************************************************************************/
{
   memset (HashTab[white], 0, HashSize * sizeof (HashSlot));
   memset (HashTab[black], 0, HashSize * sizeof (HashSlot));
}


void PTClear ()
/****************************************************************************
 *   
 *  Zero out the transposition table.
 *
 ****************************************************************************/
{
   memset (PawnTab[white], 0, PAWNSLOTS * sizeof (PawnSlot));
   memset (PawnTab[black], 0, PAWNSLOTS * sizeof (PawnSlot));
}
