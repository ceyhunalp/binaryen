;; NOTE: Assertions have been generated by update_lit_checks.py --all-items and should not be edited.
;; RUN: foreach %s %t wasm-opt --nominal --gto -all -S -o - | filecheck %s
;; (remove-unused-names is added to test fallthrough values without a block
;; name getting in the way)

(module
  ;; A struct with a field that is never read or written, so it can be
  ;; removed.

  (type $struct (struct (field (mut funcref))))

  (func $func (param $x (ref $struct))
  )
)
