
let n = 400; 
const t = 4; // make sure n%t == 0, actually 4 is just optimal gui wise
let A; 
let X;
let lows, highs;
let pivotVal;
let swapFlags; // true at index i if rect i came from a swap (color these yellow)
let A_type = "normal";
let alg = "smoothed"; // "strided" or "smoothed"
let nextN = n;
let speed = 200;

function resetAnimation() {
	n = nextN;
	A = [];
  X = [];
  for(let i = 0; i < n/t; i++){
    if(alg == "smoothed"){
      X[i] = Math.floor(Math.random()*t);
    }
    else{
      X[i] = 0;
    }
  }
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
		lows.push(0);
    highs.push(Math.floor(n/t) - 1)
	}
	// median of sample pivot selection
	// let pivotCandidates = [];
	// let NUM_CANDS = 10;
	// for (let i = 0; i < NUM_CANDS; i++){
	//   pivotCandidates.push(A[Math.floor(Math.random()*n)]);
	// }
	// pivotCandidates.sort();
	// pivotVal = pivotCandidates[NUM_CANDS/2];
	pivotVal = Math.random()*0.5+0.25;
	setTimeout(partitionAnimation, speed*1);
}

// function finish(vmin, vmax){
//   background(200,200,200); // grey
//   noStroke();
//   for (let i = 0; i < n; i++){
//     if(i < vmin){
//       fill(0,0,0);
//     }
//     else if (i >= vmax){
//       fill(255,255,255);
//     }
//     else{
//       fill(255,0,0);
//     }
//     rect(i*width/n, height*(1-A[i]), width/n, height*A[i]);
//   }
//   fill(0,255,0, 100);
//   rect(0,(1-pivotVal)*height,width,10);
//   setTimeout(resetAnimation, speed*20);
// }

function setup(){
	let cnv = createCanvas(window.innerWidth*0.95, window.innerHeight*0.95);
	cnv.parent("cnv-parent");
	resetAnimation();
}

function partitionAnimation(){
	let resetCt = 0;
	background(200,200,200); // grey
	noStroke();
	push();
	translate(0, height*0.25);
	scale(1,0.75);
	let vmin = n-1; let vmax = 0;
	for (let ti = 0; ti < t; ti++){
		push();
		translate(0, (ti/t)*height);
		scale(1, 1/t);

		if(lows[ti]*t + (X[lows[ti]]+ti)%t >= highs[ti]*t + (X[highs[ti]]+ti)%t){
			resetCt++;
			vmin = Math.min(vmin, highs[ti]*t + (X[highs[ti]]+ti)%t); // this is approximate!!!!!!
			vmax = Math.max(vmax, lows[ti]*t + (X[lows[ti]]+ti)%t); // this is approximate!!!!!! -- ie a bug bug bug BUG BUG BUG
		}
		else{
			if (A[lows[ti]*t + (X[lows[ti]]+ti)%t] <= pivotVal){
        lows[ti] += 1;
			}
			else if(A[highs[ti]*t + (X[highs[ti]]+ti)%t] > pivotVal){
        highs[ti] -= 1;
			}
			else{
        // console.log(X, lows, ti, t, lows[ti], X[lows[ti]]);
        // console.log(X, highs, ti, t, highs[ti], X[highs[ti]]);
        // console.log(lows[ti]*t + (X[lows[ti]]+ti)%t);
        // console.log(highs[ti]*t + (X[highs[ti]]+ti)%t);
				let tmp = A[highs[ti]*t + (X[highs[ti]]+ti)%t];
				A[highs[ti]*t + (X[highs[ti]]+ti)%t] = A[lows[ti]*t + (X[lows[ti]]+ti)%t];
				A[lows[ti]*t + (X[lows[ti]]+ti)%t] = tmp;

        if(A[lows[ti]*t + (X[lows[ti]]+ti)%t] == undefined){
          console.log("woops low , "+ ti, lows[ti], t*lows[ti]+(X[lows[ti]]+ti)%t, t*highs[ti]+(X[highs[ti]]+ti)%t);
        }
        if(A[highs[ti]*t + (X[highs[ti]]+ti)%t] == undefined){
          console.log("woops high , "+ ti, highs[ti], t*highs[ti]+(X[highs[ti]]+ti)%t, t*lows[ti]+(X[lows[ti]]+ti)%t);
        }
				swapFlags[lows[ti]*t + (X[lows[ti]]+ti)%t] = true;
				swapFlags[highs[ti]*t + (X[highs[ti]]+ti)%t] = true;
			}
		}

    let gotRed = false;
		for(let i = 0; i < n/t; i++){
      let real_idx = i*t + (X[i]+ti)%t;
			if (i == lows[ti] || i == highs[ti]){
				fill(255,0,0); // red
        gotRed = true;
			}
			else if(swapFlags[real_idx]){
				// fill(255,255,0, 100);
        fill(0,255,255,100);
			}
			else if (i < lows[ti] || i > highs[ti]) {
				fill(0,0,0); // black
			}
			else {
				fill(255,255,255); // white
			}
			rect(real_idx*width/n, height*(1-A[real_idx]), width/n, height*A[real_idx]);
		}

    if(!gotRed){
      console.log("wut +"+ti);
    }

		// pivot value
		fill(0,255,0, 250); // green
		rect(0,(1-pivotVal)*height,width,10);

		pop();
	}
	pop();

	for(let ti = 0; ti < t; ti++){
		for(let i = ti; i < n; i+=t){
			if (i == lows[ti]*t+(X[lows[ti]]+ti)%t || i == highs[ti]*t+(X[highs[ti]]+ti)%t){
				fill(255,0,0); // red
			}
			else if(swapFlags[i]){
				// fill(255,255,0, 100);
        fill(0,255,255,100);
			}
			else if (i < lows[ti]*t+(X[lows[ti]]+ti)%t || i > highs[ti]*t+(X[highs[ti]]+ti)%t) {
				fill(0,0,0); // black
			}
			else {
				fill(255,255,255); // white
			}
			rect(i*width/n, 0.25*height*(1-A[i]), width/n, 0.25*height*A[i]);
		}
	}
	// pivot value
	fill(0,255,0, 250);// green
	rect(0,(1-pivotVal)*height*0.25,width,10*0.25);


	if(resetCt == t){
    $.notify("Finished partial partition");
    setTimeout(resetAnimation, speed*35);
		// setTimeout(function(){
    //   finish(vmin, vmax);
		// }, speed*35);
	}
	else{
		setTimeout(partitionAnimation, speed*1);
	}
}
