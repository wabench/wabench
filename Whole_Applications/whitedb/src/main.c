#include <stdlib.h>
#include <stdio.h>

#include "dbapi.h"
#include "indexapi.h"

int speed2() {
  void *db, *rec;
  char *name="2";
  int i;
  db = wg_attach_database(name, 1000000000);
  if (!db) { printf("db creation failed \n"); exit(0); }
  for(i=0;i<10000000;i++) {
    rec = wg_create_record(db, 9);
    if (!rec) { printf("record creation failed \n"); exit(0); }
  }
  printf("i %d\n", i);
  wg_detach_database(db);
  wg_delete_database(name);
  return 0;
}

int speed3() {
  void *db, *rec;
  char *name="3";
  int i;
  db = wg_attach_database(name, 1000000000);
  if (!db) { printf("db creation failed \n"); exit(0); }
  for(i=0;i<10000000;i++) {
    rec = wg_create_raw_record(db, 5);
    if (!rec) { printf("record creation failed \n"); exit(0); }
  }
  printf("i %d\n", i);
  wg_detach_database(db);
  wg_delete_database(name);
  return 0;
}

int speed4() {
  void *db, *rec;
  char *name="4";
  int i;
  db = wg_attach_database(name, 1000000000);
  if (!db) { printf("db creation failed \n"); exit(0); }
  for(i=0;i<10000000;i++) {
    rec = wg_create_record(db, 5);
    if (!rec) { printf("record creation failed \n"); exit(0); }
    wg_delete_record(db, rec);
  }
  printf("i %d\n", i);
  wg_detach_database(db);
  wg_delete_database(name);
  return 0;
}

int speed5() {
  void *db, *rec;
  char *name="51";
  int i,j,count=0;
  db = wg_attach_database(name, 1000000000);
  if (!db) { printf("db creation failed \n"); exit(0); }
  for(i=0;i<10000000;i++) {
    rec = wg_create_raw_record(db, 5); 
    if (!rec) { printf("record creation failed \n"); exit(0); }          
    for (j=0;j<5;j++) {
      wg_set_new_field(db,rec,j,wg_encode_int(db,i+j));
      //count+=wg_decode_int(db,wg_get_field(db, rec, j));
    }   
  }   
  printf("i %d count %d\n", i,count);
  wg_detach_database(db);
  wg_delete_database(name);
  return 0;
}

int speed6() {
  void *db, *rec;
  char *name="69";
  int i,j;
  char* content="01234567890";
  db = wg_attach_database(name, 1000000000);
  if (!db) { printf("db creation failed \n"); exit(0); }
  for(i=0;i<1000000;i++) {
    rec = wg_create_raw_record(db, 5);
    if (!rec) { printf("record creation failed \n"); exit(0); }
    for (j=0;j<5;j++) {
      wg_set_new_field(db,rec,j,wg_encode_str(db,content,NULL));
    }
  }
  printf("i %d \n", i);
  wg_detach_database(db);
  wg_delete_database(name);
  return 0;
}

int speed7() {
  void *db, *rec;
  char *name="7";
  int i,j;
  char* content="0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
  db = wg_attach_database(name, 1000000000);
  if (!db) { printf("db creation failed \n"); exit(0); }
  for(i=0;i<1000000;i++) {
    rec = wg_create_raw_record(db, 5);
    if (!rec) { printf("record creation failed \n"); exit(0); }
    for (j=0;j<5;j++) {
      wg_set_new_field(db,rec,j,wg_encode_str(db,content,NULL));
    }
  }
  printf("i %d \n", i);
  wg_detach_database(db);
  wg_delete_database(name);
  return 0;
}

int speed8() {
  void *db, *rec;
  char *name="9";
  int i,j;
  db = wg_attach_database(name, 1000000000);
  if (!db) { printf("db creation failed \n"); exit(0); }
  for(i=0;i<1000000;i++) {
    rec = wg_create_raw_record(db, 5);
    if (!rec) { printf("record creation failed \n"); exit(0); }
    for (j=0;j<5;j++) {
      wg_set_new_field(db,rec,j,wg_encode_double(db,(double)(i+j)));
    }
  }
  printf("i %d \n", i);
  wg_detach_database(db);
  wg_delete_database(name);
  return 0;
}

int speed10() {
  void *db, *rec;
  char *name="10";
  int i,j;
  db = wg_attach_database(name, 1000000000);
  if (!db) { printf("db creation failed \n"); exit(0); }
  for(i=0;i<10000000;i++) {
    rec = wg_create_raw_record(db, 5);
    if (!rec) { printf("record creation failed \n"); exit(0); }
    wg_set_new_field(db,rec,3,wg_encode_int(db,i%100000));
  }
  printf("i %d\n", i);
  return 0;
}

int speed11() {
  void *db, *rec;
  char *name="10";
  int i=0;
  int count=0;
  wg_int encval;
  db = wg_attach_database(name, 1000000000);
  if (!db) { printf("db attaching failed \n"); exit(0); }
  encval=wg_encode_int(db,123); // encode for faster comparison in the loop
  rec=wg_get_first_record(db);
  do {
    //if (wg_decode_int(db,wg_get_field(db,rec,3))==123) count++; // a bit slower alternative
    if (wg_get_field(db,rec,3)==encval) count++;
    rec=wg_get_next_record(db,rec);
    i++;
  } while(rec!=NULL);    
  wg_free_encoded(db,encval); // have to free encval since we did not store it to db
  printf("i %d, count %d\n", i,count);
  return 0;
}

int speed12() {
  void *db, *rec;
  char *name="10";
  int tmp;
  db = wg_attach_database(name, 1000000000);
  if (!db) { printf("db creation failed \n"); exit(0); }
  tmp=wg_create_index(db,3,WG_INDEX_TYPE_TTREE,NULL,0);
  if (tmp) printf("Index creation failed\n");
  else printf("Index creation succeeded\n");
  return 0;
}

int speed13() {
  void *db, *rec;
  char *name="10";
  int i;  
  int count=0;
  wg_query *query;  
  wg_query_arg arglist[5];
  
  db = wg_attach_database(name, 1000000000);
  if (!db) { printf("db attaching failed \n"); exit(0); } 
  // outer loop is just for sensible timing: do the same thing 1000 times
  for(i=0;i<1000000;i++) { 
    arglist[0].column = 3;
    arglist[0].cond = WG_COND_EQUAL;
    arglist[0].value = wg_encode_query_param_int(db,123);
    query = wg_make_query(db, NULL, 0, arglist, 1);
    if(!query) { printf("query creation failed \n"); exit(0); } 
    while((rec = wg_fetch(db, query))) {
      count++;
      //wg_print_record(db, rec); printf("\n");
    }  
    wg_free_query(db,query);
  }  
  printf("count altogether for i %d runs: %d\n", i, count);
  return 0;
}

int speed15() {
  void *db, *rec, *firstrec, *lastrec;
  char *name="15";
  int i;  
  db = wg_attach_database(name, 1000000000);
  if (!db) { printf("db creation failed \n"); exit(0); }
  rec = wg_create_raw_record(db, 5); 
  firstrec=rec; // store for use in the end
  lastrec=rec; 
  for(i=1;i<10000000;i++) {   
    rec = wg_create_raw_record(db, 5); 
    if (!rec) { printf("record creation failed \n"); exit(0); }   
    // store a pointer to the previously built record
    wg_set_new_field(db,rec,3,wg_encode_record(db,lastrec));   
    lastrec=rec;
  }   
  // field 3 of the first record will be an encoded NULL pointer
  // which is always just (wg_int)0
  wg_set_new_field(db,firstrec,3,wg_encode_null(db,0));
  // field 2 of the first record will be a pointer to the last record
  wg_set_new_field(db,firstrec,2,wg_encode_record(db,lastrec));
  printf("i %d\n", i); 
  wg_detach_database(db);
  return 0;
}

int speed16() {
  void *db, *rec;
  char *name="15";
  int i;  
  wg_int encptr;
  db = wg_attach_database(name, 1000000000);
  if (!db) { printf("db creation failed \n"); exit(0); }
  rec=wg_get_first_record(db);  
  // get a pointer to the last record
  rec=wg_decode_record(db,wg_get_field(db,rec,2)); 
  i=1;
  while(1) {
    encptr=wg_get_field(db,rec,3); // encptr is not yet decoded
    if (encptr==(wg_int)0) break; // encoded null is always standard 0
    rec=wg_decode_record(db,encptr); // get a pointer to the previous record
    i++;
  } 
  printf("i %d\n", i);
  wg_detach_database(db);
  return 0;
}

#define DB_NAME "20"

int speed20() {
  void *db, *rec, *ctrlrec, *firstrec, *lastrec;
  char *name=DB_NAME;
  int i;  
  int lstlen=10000000; // total nr of elems in list
  int ptrfld=3; // field where a pointer is stored  
  int segnr=1; // total number of segments
  int midcount=0; // middle ptr count  
  int midlasti=0; // last i where midpoint stored
  int midseglen; // mid segment length
  wg_int tmp;

  /*if (argc>=2) {
    segnr=atoi(argv[1]);
  }*/
  segnr = 20;  
  printf("creating a list with %d segments \n",segnr);
  midseglen=lstlen/segnr; // mid segment length
        
  db = wg_attach_database(name, 1000000000);
  if (!db) { printf("db creation failed \n"); exit(0); }
  ctrlrec = wg_create_record(db, 1000); // this will contain info about the list
  // build the list
  firstrec=wg_create_raw_record(db, 5); 
  lastrec=firstrec;
  // next ptr from the last record is an encoded NULL pointer
  wg_set_new_field(db,firstrec,ptrfld,wg_encode_null(db,0));  
  wg_set_new_field(db,firstrec,1,wg_encode_int(db,lstlen));
  for(i=1;i<lstlen;i++) {   
    rec = wg_create_raw_record(db, 5);
    if (!rec) { printf("record creation failed \n"); exit(0); }   
    // store a pointer to the previously built record
    wg_set_new_field(db,rec,ptrfld,wg_encode_record(db,lastrec));
    wg_set_new_field(db,rec,1,wg_encode_int(db,lstlen-i));
    lastrec=rec;
    // check if the pointer should be stored as as a midptr
    // observe the last segment may be slightly longer
    // example: lstlen=11, midnr=1: midseglen=5, midpoint0=5, at i=10 11-10<5
    // example: lstlen=10, midnr=2: midseglen=3, midpoint0=3, midpoint1=6 at i=9 10-9<3
    if (i-midlasti==midseglen && lstlen-i>=midseglen) {
      // this lst is built from end to beginning
      wg_set_field(db,ctrlrec,2+(segnr-1)-midcount,wg_encode_record(db,rec));
      //printf("\nmidpoint %d at i %d to field %d val %d",midcount,i,2+(segnr-1)-midcount, 
      //  (int)(db,wg_get_field(db,ctrlrec,2+(segnr-1)-midcount)));
      midlasti=i;
      midcount++;
    }          
  } 
  // set ctrlrec fields: type,ptr field,first pointer,midpointer1,midpointer2,...
  wg_set_field(db,ctrlrec,0,wg_encode_int(db,1)); // type not used
  wg_set_field(db,ctrlrec,1,wg_encode_int(db,ptrfld)); // ptrs at field 3
  wg_set_field(db,ctrlrec,2,wg_encode_record(db,lastrec)); // lst starts here
  wg_set_field(db,ctrlrec,2+segnr,wg_encode_record(db,firstrec)); // last record in a list
  printf("\nfinal i %d\n", i);
  printf("\nctrl rec ptr fld val %d \n",wg_decode_int(db,wg_get_field(db,ctrlrec,1)));
  for(i=0;i<1000;i++) {
    tmp=wg_get_field(db,ctrlrec,2+i);
    if (!(int)tmp) break;
    //printf("ptr %d value %d content %d\n",i,(int)tmp,
    //   wg_decode_int(db,wg_get_field(db,wg_decode_record(db,tmp),1)) ); 
  }    
  wg_detach_database(db);
  return 0;
}

#define RECORD_HEADER_GINTS 3
#define wg_field_addr(db,record,fieldnr) (((wg_int*)record)+RECORD_HEADER_GINTS+fieldnr)

#define MAX_THREADS 100 
#define DB_NAME "20"

struct thread_data{
  int    thread_id; // 0,1,..
  void*  db;     // db handler
  wg_int fptr;   // first pointer
  wg_int lptr;   // last pointer
  int    ptrfld; // pointer field in rec
  int    res;    // stored by thread
};

static void *process(void *targ) {
  struct thread_data *tdata; 
  void *db,*rec,*lptr;
  wg_int encptr;
  int i,tid,ptrfld;
       
  tdata=(struct thread_data *) targ;    
  tid=tdata->thread_id;
  db=tdata->db;
  ptrfld=tdata->ptrfld;  
  rec=wg_decode_record(db,tdata->fptr);
  lptr=wg_decode_record(db,tdata->lptr);
  
  printf("thread %d starts, first el value %d \n",tid,
         wg_decode_int(db,wg_get_field(db,rec,1)));
  i=1;
  while(1) {
    encptr=*(wg_field_addr(db,rec,ptrfld)); // encptr is not yet decoded   
    rec=wg_decode_record(db,encptr); // get a pointer to the previous record
    if (rec==lptr) break;
    i++;    
  } 
  tdata->res=i; 
  printf ("thread %d finishing with res %d \n",tid,i);
  //pthread_exit((void *) tid);
}

int speed21() {
  void *db, *ctrl, *rec;
  char *name=DB_NAME;
  int i,ptrfld,ptrs,rc;  
  wg_int encptr,tmp,first,next,last;
  //pthread_t threads[MAX_THREADS];   
  struct thread_data tdata[MAX_THREADS];
  //pthread_attr_t attr;
  long tid;

  db = wg_attach_database(name, 1000000000);
  if (!db) { printf("db creation failed \n"); exit(0); }
  
  // get values from cntrl record
  ctrl=wg_get_first_record(db);  
  ptrfld=wg_decode_int(db,wg_get_field(db,ctrl,1));
  first=wg_get_field(db,ctrl,2);
  for(ptrs=0;ptrs<10000;ptrs++) {
    tmp=wg_get_field(db,ctrl,3+ptrs);
    if ((int)tmp==0) break;          
    last=tmp;
  }
  printf("\nsegments found: %d \n",ptrs);
  if (ptrs>=MAX_THREADS) {
    printf("List segment nr larger than MAX_THREADS, exiting\n");
    wg_detach_database(db);  
    //pthread_exit(NULL);
    return 0;
  }
  // prepare and create threads
  //pthread_attr_init(&attr);
  //pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  
  for(tid=0;tid<ptrs;tid++) {   
    first=wg_get_field(db,ctrl,2+tid);
    next=wg_get_field(db,ctrl,2+tid+1);
    tdata[tid].thread_id=tid;
    tdata[tid].db=db;
    tdata[tid].fptr=first;
    tdata[tid].lptr=next;
    tdata[tid].ptrfld=ptrfld;
    tdata[tid].res=0;    
    process((void *)&tdata[tid]);
    //rc=pthread_create(&threads[tid], &attr, process, (void *) &tdata[tid]);    
  }  
 
  // wait for all threads to finish
  //for(tid=0;tid<ptrs;tid++) {
  //  pthread_join(threads[tid],NULL);
    //printf("thread %d finished with res %d\n",
    //        tdata[tid].thread_id,tdata[tid].res);
  //}
  printf("\nall threads finished\n"); 
  wg_detach_database(db);  
  //pthread_exit(NULL);
  return 0;
}

int main(int argc, char *argv[]) {

  // run tests
  speed2();

  speed3();

  speed4();

  speed5();

  speed6();

  speed7();

  speed8();

  speed10();

  speed11();

  speed12();

  speed13();

  speed15();

  speed16();

  //speed20();

  //speed21();

  return 0;
}
