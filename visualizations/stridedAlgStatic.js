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
let specificPvmin = 0;
let redSlides = [2,3,3.5];

function initA(){
		for (let i = 0; i < n; i++){
			A[i] = Math.random();
		}
}

function setAndSaveA(new_slide){
	setA(new_slide);
	setTimeout(function(){save(("stridedAlgSim_"+new_slide).replace(".","_"))}, 500);
}

function setA(new_slide){
	which_slide = new_slide;
	if(which_slide == 1 || which_slide == 2){
		// we don't want a new A, but we want A to be messed up, so do some random swaps!
		for(let pi = 0; pi < g; pi++){
			for(let i = pi; i < n; i+=g){
				for(let j = pi; j < n; j+=g){
					if(Math.random()<0.25){
						let tmp = A[i];
						A[i] = A[j];
						A[j] = tmp;
					}
				}
			}
		}
	}
	else if(which_slide == 3){
		let low = pidx;
		let high = (s-1)*g+pidx;
		while(low < high){
			while(A[low] <= pivotValue && low < high){
				low+=g;
			}
			while(A[high] > pivotValue && low < high){
				high -= g;
			}
			let tmp = A[low];
			A[low] = A[high];
			A[high] = tmp;
		}
		if(A[low] <= pivotValue){
			low += g;
		}
	}
	else if(which_slide == 3.5){
		let low = pidx;
		let high = (s-1)*g+pidx;
		while(low < high){
			while(A[low] <= pivotValue && low < high){
				low+=g;
			}
			while(A[high] > pivotValue && low < high){
				high -= g;
			}
			let tmp = A[low];
			A[low] = A[high];
			A[high] = tmp;
		}
		if(A[low] <= pivotValue){
			low += g;
		}
		specificPvmin = low;
	}
	else if(which_slide == 4){
		vs = [];
		for(let pi = 0; pi < g; pi++){
			let low = pi;
			let high = (s-1)*g+pi;
			while(low < high){
				while(A[low] <= pivotValue && low < high){
					low+=g;
				}
				while(A[high] > pivotValue && low < high){
					high -= g;
				}
				let tmp = A[low];
				A[low]=A[high];
				A[high]=tmp;
			}
			if(A[low] <= pivotValue){
				low += g;
			}
			vs.push(low);
		}
	}
	else if(which_slide == 4.5){
		vs = [];
		for(let pi = 0; pi < g; pi++){
			let low = pi;
			let high = (s-1)*g+pi;
			while(low < high){
				while(A[low] <= pivotValue && low < high){
					low+=g;
				}
				while(A[high] > pivotValue && low < high){
					high -= g;
				}
				let tmp = A[low];
				A[low]=A[high];
				A[high]=tmp;
			}
			if(A[low] <= pivotValue){
				low += g;
			}
			vs.push(low);
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
			if(j == pidx && redSlides.indexOf(which_slide) != -1){
				fill(255,0,0); // red
			}
			else{
				fill(255,255,255); // white
			}
			if(which_slide == 4 || which_slide == 4.5){
				for(let iii = 0; iii < vs.length; iii++){
					if(i*g+j == vs[iii]){
						fill(0,0,255); // blue
					}
				}
			}
			if(which_slide == 3.5){
				if(i*g+j == specificPvmin){
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
