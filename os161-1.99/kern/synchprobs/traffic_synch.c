#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>
#include <array.h>
/* 
 * This simple default synchronization mechanism allows only vehicle at a time
 * into the intersection.   The intersectionSem is used as a a lock.
 * We use a semaphore rather than a lock so that this code will work even
 * before locks are implemented.
 */


// AV struct def

typedef struct AV
{
  Direction origin;
  Direction destination;
} AV;

//forward declarations
bool right_turn(AV* v);
int conditionCheck(AV* enter, AV* present);
bool loopThrough (AV * vehicle);

//condition variable declaration
static struct cv* intersectionCV;

//lock declaration
static struct lock* intersectionLock;

//array declaration
struct array* vehicles; 

volatile int count = 0; //checking count for debug purposes


/* 
 * The simulation driver will call this function once before starting
 * the simulation
 *
 * You can use it to initialize synchronization and other variables.
 * 
 */


void
intersection_sync_init(void)
{
  kprintf("init start\n");

  intersectionCV = cv_create("intersectionCV");
  intersectionLock = lock_create("intersectionLock");
  vehicles = array_create();
  array_init(vehicles);

  if(intersectionCV == NULL || intersectionLock == NULL || vehicles == NULL) {
    panic("hell brokeloose");
  }
  //kprintf("init success\n");
  return;
}

/* 
 * The simulation driver will call this function once after
 * the simulation has finished
 *
 * You can use it to clean up any synchronization and other variables.
 *
 */
void
intersection_sync_cleanup(void)
{
  KASSERT(intersectionLock != NULL);
  KASSERT(intersectionCV != NULL);
  KASSERT(vehicles != NULL);

  lock_destroy(intersectionLock);
  cv_destroy(intersectionCV);
  array_destroy(vehicles);
}

bool
right_turn(AV* v) {
  //KASSERT(v != NULL);
  if (((v->origin == west) && (v->destination == south)) ||
      ((v->origin == south) && (v->destination == east)) ||
      ((v->origin == east) && (v->destination == north)) ||
      ((v->origin == north) && (v->destination == west))) {

    return true;
  } else {
    return false;
  }
}

int conditionCheck(AV* enter, AV* present) {

  if (enter->origin == present->origin)
  {
    return 1;
  }
  else if ((enter->origin == present->destination) && (enter->destination == present->origin))
  {
    return 1;
  }
  else if ((enter->destination != present->destination) && (right_turn(enter) || right_turn(present)))
  {
    return 1;
  }
  else return 0;
}


/*
 * The simulation driver will call this function each time a vehicle
 * tries to enter the intersection, before it enters.
 * This function should cause the calling simulation thread 
 * to block until it is OK for the vehicle to enter the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle is arriving
 *    * destination: the Direction in which the vehicle is trying to go
 *
 * return value: none
 */

bool loopThrough (AV * vehicle) {
  for (unsigned int i = 0; i < array_num(vehicles); ++i)
  { 
    if (!conditionCheck(vehicle, array_get(vehicles, i)))
    {
      cv_wait(intersectionCV, intersectionLock);
      return false;
    }
  }
  
  KASSERT(lock_do_i_hold(intersectionLock));
  //kprintf("going to enter intersection. Origin:%d destination:%d. Count: %d\n", vehicle->origin, vehicle->destination,count);
  array_add(vehicles, vehicle, NULL);
  count++;
  return true;
}

void
intersection_before_entry(Direction origin, Direction destination) 
{

  KASSERT(intersectionLock != NULL);
  KASSERT(intersectionCV != NULL);
  KASSERT(vehicles != NULL);

  lock_acquire(intersectionLock);


  AV* vehicle = kmalloc(sizeof(struct AV));
  KASSERT(vehicle != NULL);
  vehicle->origin = origin;
  vehicle->destination = destination;
  
  while(!loopThrough(vehicle)) {
    count += 0;
  }
  lock_release(intersectionLock);
}


/*
 * The simulation driver will call this function each time a vehicle
 * leaves the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle arrived
 *    * destination: the Direction in which the vehicle is going
 *
 * return value: none
 */

void
intersection_after_exit(Direction origin, Direction destination) 
{

  KASSERT(intersectionLock != NULL);
  KASSERT(intersectionCV != NULL);
  KASSERT(vehicles != NULL);

  lock_acquire(intersectionLock);
  
  for (unsigned int i = 0; i < array_num(vehicles); ++i)
  {
    AV* curr = array_get(vehicles,i); 
    if ((curr->origin == origin) &&(curr->destination == destination))
    {
      array_remove(vehicles, i);
      cv_broadcast(intersectionCV, intersectionLock);
      count--;
      //kprintf("signalled that leaving Origin:%d destination:%d, count:%d\n", origin, destination,count);
      break;
    }
  }
  lock_release(intersectionLock);
  //kprintf("released lock \n");
}
