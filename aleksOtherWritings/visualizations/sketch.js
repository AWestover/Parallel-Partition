
let n = 300;
let t = 4;
let A; 
let lows, highs;
let pivotVal;
let swapFlags; // true at index i if rect i came from a swap (color these yellow)
let A_type = "normal";

function resetAnimation() {
	A = [];
	swapFlags = [];
	for(let i = 0; i < n; i++){
		if(A_type == "normal"){
			A.push(Math.random());
		}
		else if(A_type == "predecessor heavy"){
			if(Math.random() < 0.8){
				A.push(0.2+Math.random()*0.8);
			}
			else{
				A.push(Math.random());
			}
		}
		else if(A_type == "adversarial"){
			if(i % t == 0){
				A.push(Math.random()*0.3);
			}
			else if(i% t == t-1){
				A.push(Math.random()*0.3+0.7);
			}
			else{
				A.push(Math.random()*0.4+0.3);
			}
		}
		swapFlags.push(false);
	}
	lows = [];
	highs = [];
	for (let ti = 0; ti < t; ti++){
		lows.push(ti);
		let lowerBound = t*(Math.floor((n)/t)-1); // Im not sure this is quite right...
		if(lowerBound+ti < n){
			highs.push(lowerBound+ti);
		}
		else{
			highs.push(lowerBound);
		}
	}
	let pivotCandidates = [];
	let NUM_CANDS = 10;
	for (let i = 0; i < NUM_CANDS; i++){
		pivotCandidates.push(A[Math.floor(Math.random()*n)]);
	}
	pivotCandidates.sort();
	pivotVal = pivotCandidates[NUM_CANDS/2];
	setTimeout(partitionAnimation, 200);
}

function finish(vmin, vmax){
	background(200,200,200); // grey
	noStroke();
	for (let i = 0; i < n; i++){
		if(i < vmin){
			fill(0,0,0);
		}
		else if (i >= vmax){
			fill(255,255,255);
		}
		else{
			fill(255,0,0);
		}
		rect(i*width/n, height*(1-A[i]), width/n, height*A[i]);
	}
	fill(0,255,0, 100);
	rect(0,(1-pivotVal)*height,width,10);
	setTimeout(resetAnimation, 7000);
}

function setup(){
	let cnv = createCanvas(window.innerWidth*0.95, window.innerHeight*0.95);
	cnv.parent("cnv-parent");
	resetAnimation();
}

function partitionAnimation(){
	let resetCt = 0;
	background(200,200,200); // grey
	noStroke();
	let vmin = n-1; let vmax = 0;
	for (let ti = 0; ti < t; ti++){
		push();
		translate(0, (ti/t)*height);
		scale(1, 1/t);
		if (A[lows[ti]] <= pivotVal){
			lows[ti]+=t;
		}
		else if(A[highs[ti]]>pivotVal){
			highs[ti]-=t;
		}
		else{
			let tmp = A[highs[ti]];
			A[highs[ti]] = A[lows[ti]];
			A[lows[ti]] = tmp;
			swapFlags[lows[ti]] = true;
			swapFlags[highs[ti]] = true;
		}

		if(lows[ti] >= highs[ti]){
			resetCt ++;
			vmin = Math.min(vmin, highs[ti]);// this is approximate!!!!!!
			vmax = Math.max(vmax, lows[ti]);// this is approximate!!!!!! -- ie a bug bug bug BUG BUG BUG
		}
		else{
			for(let i = ti; i < n; i+=t){
				if (i == lows[ti] || i == highs[ti]){
					fill(255,0,0); // red
				}
				else if(swapFlags[i]){
					fill(255,255,0, 100);
				}
				else if (i < lows[ti] || i > highs[ti]) {
					fill(0,0,0); // black
				}
				else {
					fill(255,255,255); // white
				}
				rect(i*width/n, height*(1-A[i]), width/n, height*A[i]);
			}
			// pivot value
			fill(0,255,0, 100);
			rect(0,(1-pivotVal)*height,width,10);
		}

		pop();
	}
	if(resetCt == t){
		$.notify("Finished partial partition");
		finish(vmin, vmax);
	}
	else{
		setTimeout(partitionAnimation, 200);
	}
}

