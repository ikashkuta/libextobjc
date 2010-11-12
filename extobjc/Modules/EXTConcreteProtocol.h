/*
 *  EXTConcreteProtocol.h
 *  extobjc
 *
 *  Created by Justin Spahr-Summers on 2010-11-09.
 *  Released into the public domain.
 */

#import <objc/runtime.h>
#import <stdio.h>
#import "metamacros.h"

/**
 * Used to list methods with concrete implementations within a \@protocol
 * definition.
 *
 * Semantically, this is equivalent to using \@optional, but is recommended for
 * documentation reasons. Although concrete protocol methods are optional in the
 * sense that conforming objects don't need to implement them, they are,
 * however, always guaranteed to be present.
 */
#define concrete \
	optional

/**
 * Defines a "concrete protocol," which can provide default implementations of
 * methods within protocol \a NAME. A \@protocol block should exist in a header
 * file, and a corresponding \@concreteprotocol block in an implementation file.
 * Any object that declares itself to conform to protocol \a NAME will receive
 * its method implementations \e only if no method by the same name already
 * exists.
 *
 * @code

// MyProtocol.h
@protocol MyProtocol
@required
	- (void)someRequiredMethod;

@optional
	- (void)someOptionalMethod;

@concrete
	- (BOOL)isConcrete;

@end

// MyProtocol.m
@concreteprotocol(MyProtocol)
- (BOOL)isConcrete {
  	return YES;
}

@end

 * @endcode
 *
 * If a concrete protocol \c X conforms to another concrete protocol \c Y, any
 * concrete implementations in \c X will be prioritized over those in \c Y. In
 * other words, if both protocols provide a default implementation for a method,
 * the one from \c X (the most descendant) is the one that will be loaded into
 * any class that conforms to \c X. Classes that conform to \c Y will naturally
 * only use the implementations from \c Y.
 *
 * To perform tasks when a concrete protocol is loaded, use the \c +initialize
 * method. This method in a concrete protocol is treated similarly to \c +load
 * in categories – it will be executed exactly once per concrete protocol, and
 * is not added to any classes which receive the concrete protocol's methods.
 * Note, however, that the protocol's methods may not have been added to
 * conforming classes at the time that \c +initialize is invoked.
 *
 * @warning You should not invoke methods against \c super in the implementation
 * of a concrete protocol, as the superclass may not be the type you expect (and
 * may not even inherit from \c NSObject).
 */
#define concreteprotocol(NAME) \
	/*
	 * create a class that simply contains all the methods used in this protocol
	 *
	 * it also conforms to the protocol itself, to help with static typing (for
	 * instance, calling another protocol'd method on self) – this doesn't cause
	 * any problems with the injection, since it's always done non-destructively
	 */ \
	interface NAME ## _MethodContainer : NSObject < NAME > {} \
	@end \
	\
	@implementation NAME ## _MethodContainer \
	/*
	 * when this class is loaded into the runtime, add the concrete protocol
	 * into the list we have of them
	 */ \
	+ (void)load { \
		/*
		 * passes the actual protocol as the first parameter, then this class as
		 * the second
		 */ \
		if (!ext_addConcreteProtocol(objc_getProtocol(# NAME), self)) \
			fprintf(stderr, "ERROR: Could not load concrete protocol %s", # NAME); \
	} \
	\
	/*
	 * using the "constructor" function attribute, we can ensure that this
	 * function is executed only AFTER all the Objective-C runtime setup (i.e.,
	 * after all +load methods have been executed)
	 */ \
	__attribute__((constructor)) \
	static void ext_ ## NAME ## _inject (void) { \
		/*
		 * use this injection point to mark this concrete protocol as ready for
		 * loading
		 */ \
		ext_loadConcreteProtocol(objc_getProtocol(# NAME)); \
		\
		/*
		 * use a message send to invoke +initialize if it's implemented
		 * set up an autorelease pool so that normal Objective-C stuff can be
		 * used in such a method
		 */ \
		NSAutoreleasePool *pool_ = [NSAutoreleasePool new]; \
		(void)[NAME ## _MethodContainer class]; \
		[pool_ drain]; \
	}

/*** implementation details follow ***/
BOOL ext_addConcreteProtocol (Protocol *protocol, Class methodContainer);
void ext_loadConcreteProtocol (Protocol *protocol);

