export namespace main {
	
	export class APIPaymentResponse {
	    id: string;
	    status: string;
	    confirmation_url: string;
	
	    static createFrom(source: any = {}) {
	        return new APIPaymentResponse(source);
	    }
	
	    constructor(source: any = {}) {
	        if ('string' === typeof source) source = JSON.parse(source);
	        this.id = source["id"];
	        this.status = source["status"];
	        this.confirmation_url = source["confirmation_url"];
	    }
	}
	export class PaymentMethod {
	    cardLast4: string;
	    cardBrand: string;
	    cardExpiry: string;
	
	    static createFrom(source: any = {}) {
	        return new PaymentMethod(source);
	    }
	
	    constructor(source: any = {}) {
	        if ('string' === typeof source) source = JSON.parse(source);
	        this.cardLast4 = source["cardLast4"];
	        this.cardBrand = source["cardBrand"];
	        this.cardExpiry = source["cardExpiry"];
	    }
	}
	export class PaymentRecord {
	    id: number;
	    userId: string;
	    amount: number;
	    plan: string;
	    status: string;
	    // Go type: time
	    createdAt: any;
	
	    static createFrom(source: any = {}) {
	        return new PaymentRecord(source);
	    }
	
	    constructor(source: any = {}) {
	        if ('string' === typeof source) source = JSON.parse(source);
	        this.id = source["id"];
	        this.userId = source["userId"];
	        this.amount = source["amount"];
	        this.plan = source["plan"];
	        this.status = source["status"];
	        this.createdAt = this.convertValues(source["createdAt"], null);
	    }
	
		convertValues(a: any, classs: any, asMap: boolean = false): any {
		    if (!a) {
		        return a;
		    }
		    if (a.slice && a.map) {
		        return (a as any[]).map(elem => this.convertValues(elem, classs));
		    } else if ("object" === typeof a) {
		        if (asMap) {
		            for (const key of Object.keys(a)) {
		                a[key] = new classs(a[key]);
		            }
		            return a;
		        }
		        return new classs(a);
		    }
		    return a;
		}
	}
	export class Server {
	    id: string;
	    country: string;
	    city: string;
	    flag: string;
	    config: string;
	    isPremium: boolean;
	    latency: number;
	
	    static createFrom(source: any = {}) {
	        return new Server(source);
	    }
	
	    constructor(source: any = {}) {
	        if ('string' === typeof source) source = JSON.parse(source);
	        this.id = source["id"];
	        this.country = source["country"];
	        this.city = source["city"];
	        this.flag = source["flag"];
	        this.config = source["config"];
	        this.isPremium = source["isPremium"];
	        this.latency = source["latency"];
	    }
	}
	export class Subscription {
	    id: number;
	    userId: string;
	    plan: string;
	    status: string;
	    // Go type: time
	    startDate: any;
	    // Go type: time
	    expiryDate: any;
	    autoRenew: boolean;
	    // Go type: time
	    lastPayment: any;
	    price: number;
	
	    static createFrom(source: any = {}) {
	        return new Subscription(source);
	    }
	
	    constructor(source: any = {}) {
	        if ('string' === typeof source) source = JSON.parse(source);
	        this.id = source["id"];
	        this.userId = source["userId"];
	        this.plan = source["plan"];
	        this.status = source["status"];
	        this.startDate = this.convertValues(source["startDate"], null);
	        this.expiryDate = this.convertValues(source["expiryDate"], null);
	        this.autoRenew = source["autoRenew"];
	        this.lastPayment = this.convertValues(source["lastPayment"], null);
	        this.price = source["price"];
	    }
	
		convertValues(a: any, classs: any, asMap: boolean = false): any {
		    if (!a) {
		        return a;
		    }
		    if (a.slice && a.map) {
		        return (a as any[]).map(elem => this.convertValues(elem, classs));
		    } else if ("object" === typeof a) {
		        if (asMap) {
		            for (const key of Object.keys(a)) {
		                a[key] = new classs(a[key]);
		            }
		            return a;
		        }
		        return new classs(a);
		    }
		    return a;
		}
	}
	export class User {
	    id: string;
	    email: string;
	    // Go type: time
	    createdAt: any;
	
	    static createFrom(source: any = {}) {
	        return new User(source);
	    }
	
	    constructor(source: any = {}) {
	        if ('string' === typeof source) source = JSON.parse(source);
	        this.id = source["id"];
	        this.email = source["email"];
	        this.createdAt = this.convertValues(source["createdAt"], null);
	    }
	
		convertValues(a: any, classs: any, asMap: boolean = false): any {
		    if (!a) {
		        return a;
		    }
		    if (a.slice && a.map) {
		        return (a as any[]).map(elem => this.convertValues(elem, classs));
		    } else if ("object" === typeof a) {
		        if (asMap) {
		            for (const key of Object.keys(a)) {
		                a[key] = new classs(a[key]);
		            }
		            return a;
		        }
		        return new classs(a);
		    }
		    return a;
		}
	}

}

