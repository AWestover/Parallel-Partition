let which_slide = 1;
let n = 8*12;
// let b = 1;
let g = 8;
let s = n/g;
let squareSide = 0.8*1000/s;
let cacheLineWidth = squareSide/g;
let pidx = 3;
let pivotValue = 0.55;

let A = [];
let vs = [];
let X = [];
let specificUvmin = 0;
let redSlides = [2,3,3.5];

function initA(){
		A = [];
		X = [];
		for (let i = 0; i < n; i++){
			A.push(Math.random());
		}
		for(let i = 0; i < s; i++){
			X.push(Math.floor(Math.random()*g));
		}
}

function setAndSaveA(new_slide){
	setA(new_slide);
	setTimeout(function(){save(("smoothedAlgSim_"+new_slide).replace(".","_"))}, 500);
}

function setA(new_slide){
	which_slide = new_slide;
	if(which_slide == 1 || which_slide == 2){
		// we don't want a new A, but we want A to be messed up, so do some random swaps!
		for(let pi = 0; pi < g; pi++){
			for(let i = 0; i < s; i++){
				for(let j = 0; j < n; j++){
					let ii = g*i+((X[i]+pi)%g);
					let jj = g*j+((X[j]+pi)%g);
					if(Math.random()<0.25){
						let tmp = A[ii];
						A[ii] = A[jj];
						A[jj] = tmp;
					}
				}
			}
		}
	}
	else if(which_slide == 3){
		let low = 0;
		let high = s-1;
		while(low < high){
			while(A[g*low + ((X[low]+pidx)%g)] <= pivotValue && low < high){
				low++;
			}
			while(A[g*high + ((X[high]+pidx)%g)] > pivotValue && low < high){
				high--;
			}
			let tmp = A[g*low + ((X[low]+pidx)%g)];
			A[g*low + ((X[low]+pidx)%g)] = A[g*high + ((X[high]+pidx)%g)];
			A[g*high + ((X[high]+pidx)%g)] = tmp;
			//console.log("low: "+low + " high: " + high);
		}
		if(A[g*low + ((X[low]+pidx)%g)] <= pivotValue){
			low ++;
		}
		let lowInA = g*low + ((X[low]+pidx)%g);
	}
	else if (which_slide == 3.5){
		let low = 0;
		let high = s-1;
		while(low < high){
			while(A[g*low + ((X[low]+pidx)%g)] <= pivotValue && low < high){
				low++;
			}
			while(A[g*high + ((X[high]+pidx)%g)] > pivotValue && low < high){
				high--;
			}
			let tmp = A[g*low + ((X[low]+pidx)%g)];
			A[g*low + ((X[low]+pidx)%g)] = A[g*high + ((X[high]+pidx)%g)];
			A[g*high + ((X[high]+pidx)%g)] = tmp;
			//console.log("low: "+low + " high: " + high);
		}
		if(A[g*low + ((X[low]+pidx)%g)] <= pivotValue){
			low ++;
		}
		let lowInA = g*low + ((X[low]+pidx)%g);
		specificUvmin = lowInA;
	}
	else if(which_slide == 4){
		vs = [];
		for(let pi = 0; pi < g; pi++){
			let low = 0;
			let high = s-1;
			while(low < high){
				while(A[g*low + ((X[low]+pi)%g)] <= pivotValue && low < high){
					low++;
				}
				while(A[g*high + ((X[high]+pi)%g)] > pivotValue && low < high){
					high--;
				}
				let tmp = A[g*low + ((X[low]+pi)%g)];
				A[g*low + ((X[low]+pi)%g)] = A[g*high + ((X[high]+pi)%g)];
				A[g*high + ((X[high]+pi)%g)] = tmp;
			}
			if(A[g*low + ((X[low]+pi)%g)] <= pivotValue){
				low ++;
			}
			let lowInA = g*low + ((X[low]+pi)%g);
			vs.push(lowInA);
		}
	}
	else if(which_slide == 4.5){
		vs = [];
		for(let pi = 0; pi < g; pi++){
			let low = 0;
			let high = s-1;
			while(low < high){
				while(A[g*low + ((X[low]+pi)%g)] <= pivotValue && low < high){
					low++;
				}
				while(A[g*high + ((X[high]+pi)%g)] > pivotValue && low < high){
					high--;
				}
				let tmp = A[g*low + ((X[low]+pi)%g)];
				A[g*low + ((X[low]+pi)%g)] = A[g*high + ((X[high]+pi)%g)];
				A[g*high + ((X[high]+pi)%g)] = tmp;
			}
			if(A[g*low + ((X[low]+pi)%g)] <= pivotValue){
				low ++;
			}
			let lowInA = g*low + ((X[low]+pi)%g);
			vs.push(lowInA);
		}
		sort(vs);
		let vmin = vs[0];
		let vmax = vs[vs.length-1];
		vs = [vmin, vmax];
	}
	else if(which_slide == 5){
		let low = 0;
		let high = n-1;
		while(low < high){
			while(A[low]<= pivotValue && low < high){
				low ++;
			}
			while(A[high] > pivotValue&& low < high){
				high --;
			}
			let tmp = A[low];
			A[low]=A[high];
			A[high]=tmp;
		}
		if(A[low]<= pivotValue){
			low++;
		}
	}
}

function setup(){
	createCanvas(1000,250);
	strokeWeight(1);
	initA();
	setA(which_slide);
}

function draw(){
	background(255);
	for (let i = 0; i < s; i++){
		// C_i
		let chunkX = (0.1+i)*1000/s;
		for (let j = 0; j < g; j++){
			let cacheLineX = chunkX + j*cacheLineWidth;
			if(j == ((X[i]+pidx)%g) && redSlides.indexOf(which_slide) != -1){
				fill(255,0,0);
			}
			else{
				fill(255,255,255);
			}
			if(which_slide == 4 || which_slide == 4.5){
				for(let iii = 0; iii < vs.length; iii++){
					if(i*g+j == vs[iii]){
						fill(0,0,255);
					}
				}
			}
			if(which_slide == 3.5){
				if(i*g+j == specificUvmin){
					fill(0,0,255); // blue
				}
			}
			rect(cacheLineX-cacheLineWidth, 2*squareSide*(1-A[i*g+j]), cacheLineWidth/2, 2*squareSide*A[i*g+j]);
		}
	}
	noStroke();
	fill(0,255,0); // green
	rect(0,(1-pivotValue)*squareSide*2-2,width,4);
	stroke(0,0,0); //black
}
console.log("Note: running 'save(fileName)' will save the canvas as an image");

