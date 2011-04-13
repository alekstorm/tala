# fisherQ
# Paul Boersma, August 27, 2003
# April 7, 2008: more accuracy in fisherQ because of GSL (n.b. GSL not to be used in invFisherQ)
# Computes a significance from zero, given a measured F value.
df1 = 2
df2 = 70
f = 33.59
fisherQ = fisherQ (f, df1, df2)
fisherQ$ = fixed$ (fisherQ, 20)
echo fisherQ test: 'fisherQ' 'fisherQ$'
assert fisherQ$ = "0.00000000005932714540"
for i to 10000
   assert fisherQ (randomUniform (3, 4), 1, 100000) <> undefined
endfor

echo invFisherQ
call invFisherQ 2 70 1e-14
call invFisherQ 70 2 1e-14
call invFisherQ 1 if(windows)then(100)else(100000)fi 1e-11
if not windows
	call invFisherQ 1 1 1e-14
	call invFisherQ 100000 1 1e-11
endif
call invFisherQ 100 100 1e-9
procedure invFisherQ df1 df2 precision
   # Known values.
   assert invFisherQ (0, 'df1', 'df2') = undefined
   assert invFisherQ (1, 'df1', 'df2') = 0
   # We should be able to draw a curve of invFisherQ.
   for i from 1 to 1000
      assert abs (fisherQ (invFisherQ (i/1000, df1, df2), 'df1', 'df2') - 'i'/1000) < 'precision'   ; 'i' 'df1' 'df2'
   endfor
   # Q near 0, i.e. F large: relative precision.
   for power from 4 to if windows then 147 else 150 fi
      q = 10 ^ -power
      f = invFisherQ (q, df1, df2)
      assert f <> undefined ; 'q' 'df1' 'df2'
      assert abs (fisherQ (f, 'df1', 'df2') - 'q') < 'q'*'precision'*10 ; 'f'
   endfor
   mentioned = 0
   for power from if windows then 148 else 151 fi to 307
      q = 10 ^ -power
      f = invFisherQ (q, df1, df2)
      if f = undefined and not mentioned
         printline 'df1' 'df2' 'power'
         mentioned = 1
      endif
      assert f = undefined or abs (fisherQ (f, 'df1', 'df2') - 'q') < 'q'*'precision'*300 ; 'power' 'f'
   endfor
   # Q near 1, i.e. F near zero.
   assert abs (fisherQ (invFisherQ (0.9999, df1, df2), 'df1', 'df2') - 0.9999) < 'precision'
   assert abs (fisherQ (invFisherQ (0.99999, df1, df2), 'df1', 'df2') - 0.99999) < 'precision'
   assert abs (fisherQ (invFisherQ (0.999999, df1, df2), 'df1', 'df2') - 0.999999) < 'precision'
   assert abs (fisherQ (invFisherQ (0.9999999, df1, df2), 'df1', 'df2') - 0.9999999) < 'precision'
   assert abs (fisherQ (invFisherQ (0.99999999, df1, df2), 'df1', 'df2') - 0.99999999) < 'precision'
   assert abs (fisherQ (invFisherQ (0.999999999, df1, df2), 'df1', 'df2') - 0.999999999) < 'precision'
   #
   # The inverse relationship: q values from measured f values should map back to those f values.
   #
   for power to 100
      f = 10 ^ -power
      q = fisherQ (f, df1, df2)
      assert q=1 or abs (invFisherQ ('q', 'df1', 'df2') - 'f') < precision
   endfor
   for f from 10 to 99
      q = fisherQ (f/100, df1, df2)
      assert q>1-precision or abs (invFisherQ (q, 'df1', 'df2') - 'f'/100) < precision
      assert abs (invFisherQ (fisherQ (f/10, df1, df2), 'df1', 'df2') - 'f'/10) < f/10*precision
      assert abs (invFisherQ (fisherQ (f, df1, df2), 'df1', 'df2') - 'f') < f*precision
      assert abs (invFisherQ (fisherQ (10*f, df1, df2), 'df1', 'df2') - 10*'f') < 10*f*precision
      q = fisherQ (100*f, df1, df2)
      assert q = 0 or abs (invFisherQ ('q', 'df1', 'df2') - 100*'f') < 100*f*precision
      q = fisherQ (1000*f, df1, df2)
      assert q = 0 or abs (invFisherQ ('q', 'df1', 'df2') - 1000*'f') < 1000*f*precision
   endfor
endproc
#
# Things that used to go wrong.
#
assert invFisherQ (0.13, 1, 1e9) <> undefined ; used to exceed 60 iterations
assert invFisherQ (0.159, 2, 70) <> undefined ; used to exceed 60 iterations
#
# Things that still go wrong.
#
assert fisherQ (1, 1e19, 1e19) = undefined
#
# Check that we invert better than GSL does.
#
Debug... 29   ; set invFisherQ to GSL
f = invFisherQ (0.01, 1, 10000)   ; not such an unusual case
assert "'f'" = "nan" or "'f'" = "NaN"
Debug... 0   ; use our corrected NUMridders again
f = invFisherQ (0.01, 1, 10000)   ; same case
assert "'f:5'" = "6.63743"
#
printline OK
